Introducing MUSE (beta)
-----------------

MUSE is an experimental Proof of Work blockchain with an unproven consensus
algorithm. 

  - Currency Symbol MUSE 
  - Approximately 9,5% APR long term inflation rate


No Support & No Warranty 
------------------------
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

Code is Documentation
---------------------

Rather than attempt to describe the rules of the blockchain, it is up to
each individual to inspect the code to understand the consensus rules.  

Build Instructions
------------------
These instructions apply on Fedora 24. For other distributions, make sure you have 
installed Boost 1.58-1.60 first.
For building, you need at least 4 GB of memory available.

    sudo dnf clean metadata
    sudo dnf install automake autoconf libtool make cmake gcc flex bison doxygen gettext-devel git readline-devel openssl-devel libcurl-devel ncurses-devel boost-devel boost-static gcc-c++
    mkdir -p ~/dev/MUSE
    cd ~/dev/MUSE
    git clone https://github.com/themuseblockchain/Muse-Source.git
    cd ~/dev/MUSE/Muse-Source
    git submodule update --init --recursive
    mkdir -p ~/dev/MUSE/MUSE-build
    cd ../MUSE-build
    #optionally specify Boost root directory in case it is not installed on standard paths with -DBOOST_ROOT=/location/
    cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Debug ~/dev/MUSE/Muse-Source/ 
    #optionally, install the binaries
    cmake --build . --target all -- -j 3
    
Now the binaries shall be in directory ~/dev/MUSE/MUSE-build/programs/

Seed Nodes
----------

    138.197.68.175:29092

How to Start
------------
First, start MUSE daemon:

    cd ~/dev/MUSE/MUSE-build
    ./programs/mused/mused -s 138.197.68.175:29092 --rpc-endpoint=127.0.0.1:8090
    
The daemon is ready. The JSON-RPC is listening on localhost on port 8090

How to Mine
-----------

The mining algorithm used by Muse requires the owner to have access to the private key
used by the account. This means it does not favor mining pools.

