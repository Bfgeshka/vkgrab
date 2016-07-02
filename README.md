# vkgrab
realisation of VK API via libcurl &amp; json-c

## usage
```
vk_grabber <USER|GROUP>
OR
vk_grabber -u USER
OR
vk_grabber -g GROUP
```

## features
* downloading photos in albums
* downloading wall posts
* downloading attachments from wall posts
* downloaded files are saved in structured directories
* only screen name is needed

## dependencies
* jansson
* libcurl
* POSIX-compatible OS (*nix'es, Mac, maybe Windows w/ Cygwin)
* for some operations you'll need user access token, see config.h for details

## building
- edit config.h first!
- ```$ make```
- ```# make install```
