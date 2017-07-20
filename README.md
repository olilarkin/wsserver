# ol.wsserver
## HTML5 websocket/http webserver external for Cycling74's Max
control max patches from multiple web browsers

based on [Civetweb](http://sourceforge.net/projects/civetweb/)

note: these days, your properly better off using mira/xebra.js https://github.com/Cycling74/xebra.js

### Author

Oli Larkin 2014-2015

[www.olilarkin.co.uk](http://www.olilarkin.co.uk)

Example uses interface.js by [Charlie Roberts](http://www.charlie-roberts.com/interface/)

### Binaries/Download

32/64bit binaries can be found on the [releases page](https://github.com/olilarkin/wsserver/releases)

### Building from source

The IDE projects (Xcode6 and VS2010) are set up to be built with [maxbuild](https://github.com/olilarkin/maxbuild). If you want to compile it you should first checkout maxbuild and then checkout ol.wsserver into the examples folder or a new folder at the same level.

The following commands should get you set up:

<pre><code>git clone --recursive https://github.com/olilarkin/maxbuild.git
cd maxbuild/examples
git clone --recursive https://github.com/olilarkin/wsserver.git
</code></pre>
### Tips

If you're on a local network that is blocking traffic on port 8080 you can redirect traffic from port 80 with this ipfw command in terminal (OSX)

<pre><code>/sbin/ipfw add 1000 fwd 127.0.0.1,8080 tcp from any to me 80</code></pre>

### Thanks

Civetweb and Mongoose developers, Cockos, Graham Wakefield, Mattijs Kneppers, Tim Place, JKC, Charlie Roberts

### Licence

MIT. Please see individual source code files for more licence info.

  Copyright (c) 2014 Oliver Larkin
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
