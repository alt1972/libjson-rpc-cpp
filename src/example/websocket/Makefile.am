SUBDIRS = .

AM_CPPFLAGS = \
	-Wall \
	-I$(top_srcdir)/src 
   
appldflags = -rdynamic

appldadd = \
  @CURL_LIBS@ \
  ../libjsonrpccpp.la

bin_PROGRAMS = \
  simplewebsocketserver

simplewebsocketserver_LDADD = $(appldadd)
simplewebsocketserver_LDFLAGS = $(appldflags)
simplewebsocketserver_SOURCES = simplewebsocketserver.cpp 

EXTRA_DIST = wsclient.html





