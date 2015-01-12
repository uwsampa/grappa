
Satisfying Grappa's third-party dependences without web access
--------------------------------------------------------------

If you want to build Grappa on a machine without access to the web, and that machine doesn't already have all the third-party libraries installed that Grappa needs, you'll have to provide the source archives for those dependences yourself. 

To do so, download and untar the following file in ```third-party/downloads```. Then run ```configure```, including the ```--no-downloads``` flag.

[http://grappa.cs.washington.edu/files/grappa-third-party-downloads.tar](http://grappa.cs.washington.edu/files/grappa-third-party-downloads.tar)