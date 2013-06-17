#include "HttpReader.h"
#include "curl/curl.h"

/* The content is being read into the global string
 * "httpBuf", which is pointing to a "contents" member
 * of some HttpReader object.
 */
string *httpBuf = NULL;

size_t LibcurlCallback(char* buf, size_t size, size_t nmemb, void* up) {
	if (httpBuf) {
		for (int c=0; c<size*nmemb; c++) {
			httpBuf->push_back(buf[c]);
		}
	} else {
		printf("LibcurlCallback(): No httpBuf defined!\n");
	}

	return size*nmemb;
}


// ==========================================================


HttpReader::HttpReader(string url) {
	sourceUrl = url;
}


HttpReader::~HttpReader(void) {

}


bool HttpReader::Perform() {
	contents.clear();

	CURL *curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, sourceUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &LibcurlCallback);

	httpBuf = &contents;
	curl_easy_perform(curl);
	httpBuf = NULL;

	curl_easy_cleanup(curl);

	return contents.length();
}


string& HttpReader::GetContents() {
	return contents;
}
