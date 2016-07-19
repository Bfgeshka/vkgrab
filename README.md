# vkgrab
Grab everything you can from vk.com pages! Works with VK API via libcurl &amp; jansson

## usage
```
vkgrab <USER|GROUP>
OR
vkgrab -u USER
OR
vkgrab -g GROUP
```

## features
* albums downloading
* wall posts downloading
* documents downloading
* getting attachments from wall posts (like photos, documents or links)
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
