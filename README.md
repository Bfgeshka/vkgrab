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

## windows
1. For windows at first you should [download cygwin](https://cygwin.com/install.html).
2. Be sure you've chosen **libcurl** (bin- and dev-), **wget** and **gcc-core**. Let cygwin installer download it's dependencies aswell.
3. Launch cygwin console via desktop link or any other way.
4. Install jansson. Jansson is not presented in cygwin repositories, so you'll have to compile it by yourself. Just copy/paste this sequence:
  ```
  wget http://www.digip.org/jansson/releases/jansson-2.7.tar.gz

  tar xvf jansson-2.7.tar.gz && cd jansson-2.7

  ./configure --prefix=/usr && make && make install

  cd ..
  ```
5. Clone vkgrab
  ```
  git clone https://github.com/Bfgeshka/vkgrab.git

  cd ./vkgrab

  make && make install
  ```
6. If compilation was successfull, you've installed vkgrab! type ```vkgrab -h``` for getting started.

## building
- edit config.h first!
- ```$ make```
- ```# make install```
