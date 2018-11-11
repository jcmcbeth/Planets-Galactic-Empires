# Planets: Galactic Empires
This is a version of SWR 2.0 that I had worked on between 2004 and 2005. I don't have a lot of details about this or even the complete source code. I am also not even sure if the name is correct.

What I did was take the stock SWR 2.0 and I am going to try to merge in my changes (or completely forget this exists again). My unmerged changes will be in old_src.

## How to build
Using the original tag and Ubuntu Server 18.10 use the following commands:
```bash
sudo apt-get install csh make gcc

git clone https://github.com/jcmcbeth/swr2.git
cd swr2/swr-2.0/src

make all

cd ../..
chmod +x run-swr

./run-swr &
```

Now you can telnet to the mud on port 9999.
