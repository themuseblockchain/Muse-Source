Introducing MUSE (beta)
-----------------

MUSE is a blockchain which uses Delegated Proof Of Stake (DPOS) as a consensus algorithm.

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
installed Boost 1.58-1.60 first. You also have instructions to build test net here.
For building, you need at least 4 GB of memory available.

We are testing using Fedora 25, x64

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
    
    #For production build
    cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Debug ~/dev/MUSE/Muse-Source/ 
    #For test net build
    cmake -G "Unix Makefiles" -DBUILD_MUSE_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ~/dev/MUSE/Muse-Source/
    
    #optionally, install the binaries
    cmake --build . --target all -- -j 3
    
    # For test net build, some targets may have build errors
    # Go to mused and cli_wallet folders in ~/dev/MUSE/MUSE-build/programs/ and use "make" to build them if needed.
    
Now the binaries shall be in directory ~/dev/MUSE/MUSE-build/programs/

Seed Nodes
----------

    138.197.68.175:29092 (production)
    192.34.60.157:29092  (for testnet)

How to Start
------------
First, start MUSE daemon:
(Witness will need additional config)

    cd ~/dev/MUSE/MUSE-build
    #For production:
    ./programs/mused/mused -s 138.197.68.175:29092 --replay-blockchain --rpc-endpoint=0.0.0.0:8090 --genesis-json ~/dev/MUSE/Muse-Source/genesis.json
    #--replay-blockchain may be ommited the next time you start the block chain
   
    #For testnet
    ./programs/mused/mused -s 192.34.60.157:29092 --replay-blockchain --rpc-endpoint=0.0.0.0:8090 --genesis-json ~/dev/MUSE/Muse-Source/genesis-test.json
    
The daemon is ready. The JSON-RPC is listening on localhost on port 8090

You may want to start it using screen to make server available after terminal is closed.
Install screen with dnf install screen
Start a new screen with command screen
Then run ./mused with the right parameters
You can reconnect to the screen later using screen -r

For witness setup
------------
Here are the steps to setup a witness for the testnet


    1- Go to http://192.34.60.157:3000/ to generate your keys.
    Use generate keys, write stuff, as long as it is random), just to generate a pair.
    2- Note private and public owner keys generated
    3- Prepare your test net build using instruction above
    4- Edit config: %muse build folder%/mused/witness_node_data_dir/config.ini
    5- Replace %WITNESSACCOUNT% by your account, and %PRIVWIF% by the private owner wif generated, then save file
    6- Launch node:
    cd %muse build folder%/mused/
    ./mused -s 192.34.60.157:29092 --replay-blockchain --rpc-endpoint=0.0.0.0:8090 --genesis-json ~/dev/MUSE/Muse-Source/genesis-test.json
    7- muse is running... in another session, go to %muse build folder%/cli_wallet
    8- Run cli_wallet:
    ./cli_wallet
    9- set_password $your_wallet_pass...%
    10- unlock "pass..."
    11- import_key %owner priv key%
    Use http://192.34.60.157:3000/ to reobtain keys using your password if you don't have them
    12- import key %active priv key%
    Use http://192.34.60.157:3000/ to reobtain keys using your password if you don't have them
    13- use list_my_accounts, see if you see your account
    14- get some vesting to transact...
    15- announce yourself as a witness: update_witness "test-account-2" "http://%???%" %witness_pub_key% {} true
    16- get votes... use wallet to vote, ask someone to vote
    17- at this point, you have votes, your node may be selected as an active witness and start produce blocks

