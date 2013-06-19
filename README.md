newsdisplay
===========

Terminal application for displaying RSS-feeds in real time


Build Instructions
==================

Newsdisplay builds on lib-cURL, pthread and nCurses. 

lib-cURL can be found at:
http://curl.haxx.se/download.html

nCurses can be installed via apt-get 
	
	$> sudo apt-get install libncurses5-dev

g++ is required for building ndisp 
	
	$> sudo apt-get install g++

Clone and build 
	
	$> cd ~/your/favorite/code/location/
	
	$> git clone https://github.com/pimms/newsdisplay.git 
	
	$> cd newsdisplay 
	
	$> make

Add the google news-feed  
	
	$> ./ndisp -a http://news.google.com/?output=rss 

Read the news in real time
	
	$> ./ndisp
