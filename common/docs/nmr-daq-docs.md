# The g-2 NMR-DAQ
This document is a draft of a guide to setting up and using the g-2 NMR-DAQ.  It is a work in progress as is the DAQ itself, but it's time to start. 

## Table of Contents
1. [Quick Start](#quick)
1. [Installation](#install)
1. [Basic Tutorial](#tutorial)
1. [Experiment Layout](#layout)

## <a name="quick">Quick Start</a>
#### Installation
The project has several dependencies that need to be installed first. Linux package manage has most of them.

```bash
sudo yum install mysql-devel libfftw-devel libarmadillo-devel zeromq-devel boost-devel sqlite-devel unixODBC-devel
```

##### Other Dependencies

* <a href="https://midas.triumf.ca/MidasWiki/index.php/Main_Page">MIDAS</a>
    - use the version controlled Redmine Repository, gm2midas
    - `git clone ssh://p-gm2midas@cdcvs.fnal.gov/cvs/projects/gm2midas`
* <a href="https://cdcvs.fnal.gov/redmine/projects/gm2daq/wiki">gm2daq</a>
    - `git clone --recursive ssh://p-gm2daq@cdcvs.fnal.gov/cvs/projects/gm2daq`
* <a href="https://root.cern.ch/drupal/">ROOT</a>
* <a href="https://github.com/mwsmith2/fast-daq-core">Fast-DAQ-Core</a>
    - <a href="https://github.com/mwsmith2/fid-analysis">FID Analysis</a>
    - <a href="http://www.boost.org/">boost</a>
    - <a href="http://zeromq.org/">ZeroMQ</a>

With all those taken care of, go ahead and clone the repository for the DAQ.  There are two projects that are very similar in operation.

* <a href="https://github.com/uwmuonlab/gm2-simple-daq">Simple DAQ</a>
```bash
mkdir -P ~/Applications && cd ~/Applications
git clone --recursive https://github.com/uwmuonlab/gm2-simple-daq simple-daq
```
* <a href="https://github.com/uwmuonlab/gm2-nmr-daq">g-2 NMR DAQ</a>
```bash
mkdir -P ~/Applications && cd ~/Applications
git clone --recursive https://github.com/uwmuonlab/gm2-nmr-daq gm2-nmr
```

After cloning the repository, copy common/example/expt-env to common/.expt-env and change the appropriate variables.  The most likely one needing to be altered is the EXPT_IP which should be the hostname or url for the machine running mhttpd.  Then you'll probably want to intialize the ODB to a safe size (50 MB) with `odbedit -e <expt-name> -s 50000000`.  With the ODB initialized, you can take the last step of setting up some of the ODB variables the way we want them.  This is made simpler with mhelper.

##### mhelper
This is optional, but it provides a python interface to aspects of MIDAS, the ODB, and the experiment itself. Install the command line utility like so:

```bash
cd ~/Applications/simple-daq/common/code/mhelper
sudo python setup.py install
```

With mhelper you can simply point to a json file of a certain format, such as common/examples/add_to_odb.json, and run `mhelper <path-to-json>/add_to_odb.json`.

#### Operation
If you're having issues getting all the components to build see the more in depth [installation section](#install).  With all the components properly built, you can run the DAQ in a few different ways.  The run control scripts are in the `online/bin/`, and they work by calling the configured frontends, analyzers, and MIDAS utilites to run in the their own screen.  There is another option to deploy a python utility call <a href="http://github.com/mwsmith2/mhelper">mhelper</a>. The mhelper utility figures out which experiment you're working on based on your current working directory, and it calls the run control scripts for you. Here are two equivalent ways to start the experiment.

```bash
~/Applications/online/bin/start_daq.sh
mhelper daq start
```

Confirm that the programs are running as expected by going to the address and port specfified in the `.expt-env` file (default is `nmr-daq:8080`).  The familiar MIDAS web interface should let you know that the experiment is alive.

#### Configuration
Several overall experimental parameters are stored in the ODB under Params.  Many of the frontends and slave workers also load JSON configuration files from common/config.  There will be a way to change these settings from the ODB in the future, but for now one needs to edit the JSON files manually.

#### Output
The experiment produces data based on some ODB settings.  It can produce both MIDAS files and optionally ROOT files.  ROOT output is off by default, but can be controlled with `"/Params/root-output"` in the ODB.  The data is dumped to `resources/data`. In addition to digitized data the ODB is dumped to history, the JSON configuration used is dumped to history, and runlog is dumped to the log directory.

#### Troubleshooting
The best resource when things go awry are the log files.  The MIDAS log is found at `resources/log/midas.log` by default, and the worker logs are found in `/var/log/lab-daq/`.

 
## <a name="install">Installation</a>
### Installing Scientific Linux 6
We are going start at the start here.  I made this guide based on getting a branch new computer up and running at Fermilab.  The first step was installing the OS itself which was done with a boot DVD from the Fermilab computing division. 

**Caveats:**
It was a network boot disk for me, so I couldn't do anything until internet reach the computer's location in the mezzanine.

#### Create Principal User
I went the with some collaboration favorites
*  username: newg2
*  password: [standard-password]
*  root: [collab + standard password]
*  hostname: [nmr-daq]

**Caveats:**
 *  these will depend on the machine, but it's probably best to have standards for this 
 *  the user is not a sudoer by default on this distro, but you can change that by adding to /etc/visudo
  ```newg2	ALL=(ALL)	ALL```

#### Setting Up the User Space
First thing we want to do is create some standard directories to use and keep organized.
*  ~/Applications: for midas experiments and potentially other user tools
*  ~/Packages: for user controlled packages like gm2midas and whatnot
*  ~/Workspace: a separate location for working and analyzing data
*  /home/data:  collect all of the data under one roof
	*  midas gets a directory here
	*  each midas experiment gets a subdirectory (i.e. /home/data/midas/gm2-nmr)

**Caveats:**
You may want to put the data directory in /data instead, but if you had a flash (or small separate) boot disk, you don't want to do this.  It will mount the data directory on the small disk partition

#### Install zsh (optional)
Bash will work just fine, but I've really come to like zsh.  I'd recommend trying it at least.  You can get it from the system's package manager

```bash
sudo yum install zsh
```

Then you'll want to get oh-my-zsh or prezto for configuration, https://github.com/robbyrussell/oh-my-zsh.  My favorite default theme is mrtazz, so I changed that in the ~/.zshrc file.

**Caveats:**
You want all or your zsh scripts to work in bash (zsh is mainly better for interactive use), so you want them to have the same environment.  It becomes a bit annoying to manage environment variables for both bash and zsh, so I would recommend moving shell agnostic lines to a separate file that is sourced by both .zshrc and .bashrc.

### Install MIDAS
You'll need to initialize the kerberos principal on the machine, so that you can pull from the Redmine.  Kerberos comes with Fermilab's Scientific Linux distribution, but if it isn't present in your linux installation you can probably get kerberos via the package manager (yum install krb5, apt-get install krb5, etc.) and the Fermilab configuration file can be found here: http://computing.fnal.gov/authentication/krb5conf/.

With kerberos alive and well on your new computer go ahead and clone the gm2midas package into the appropriately named ~/Packages directory.  Instructions on downloading the packages can be found https://cdcvs.fnal.gov/redmine/projects/gm2midas/wiki/, but the gist that you need for now is this.

```bash
cd ~/Packages
git clone ssh://p-gm2midas@cdcvs.fnal.gov/cvs/projects/gm2midas
```

Now we are almost ready to compile MIDAS.  I had to install mysql before compiling, but that could be different case to case.

```bash
sudo yum install mysql-devel
```

Then compiling MIDAS was fine.

```bash
cd ~/Packages/gm2midas/midas
make midas
```

The next step is setting up the appropriate environment variables for MIDAS.
 I put mine in a separate file, ~/.midas-env, sourced by .bashrc (and .zshrc for me).
 
```bash
#!/bin/bash
# A short script setting up some MIDAS environment variables

export MIDASHOME=~/Packages/gm2midas/midas
export MIDAS_ROOT=$MIDASHOME
export MIDASSYS=$MIDASHOME
export MIDAS_EXPTAB=/etc/exptab
export DAQ_DIR=~/Applications/gm2-nmr
export ONLINEDISP=~/Applications/gm2-nmr/online/display
export ROMESYS=~/Packages/gm2midas/rome

LD_LIBRARY_PATH=$MIDASSYS/linux/lib:$LD_LIBRARY_PATH

PATH=$PATH:$MIDASSYS/linux/bin
PATH=$MIDASSYS/linux/bin:$MIDASSYS/utils:$PATH
PATH=$DAQ_DIR/resources/bin:$PATH
```

### Install ROOT
We have another old friend to get up and running on our new machine, ROOT!  I ended up building ROOT from source, the guide for which is here https://root.cern.ch/drupal/content/installing-root-source.  I would recommend installing all of the optional packages, or at the very least fftw and python (see https://root.cern.ch/drupal/content/build-prerequisites).  After getting the third party libraries installed, building is simple.

```bash
cd ~/Packages/
git clone http://root.cern.ch/git/root.git
git checkout -b v5-34-08 v5-34-08
./configure --all
make -j4
```

Then we ant to add the initialization script to our shell initialization script.

```bash
. ~/Packages/root/bin/thisroot.sh
```

**Caveats:**
*  ROOT is on version 6 which should work for the code in all the DAQ packages, but it isn't the officially ROOT version we want to use yet.  
*  DO NOT DO "make install" with ROOT.  The preferred method is bringing it into the environment by sourcing a script in your .bashrc file. 

```bash
. ~/Packages/root/bin/thisroot.sh
```

*  the source line is a bit more obtuse for zsh:

```bash
pushd ~/Packages/root >/dev/null; . libexec/thisroot.sh; popd >/dev/null
```

### Set up ssh (optional)
The SLF6 distribution doesn't come with openssh set up out of the box, so I set that up with the following lines.

```bash
sudo yum install openssh-client openssh-server
ssh-keygen
chmod 600 ~/.ssh/*
chmod 700 ~/.ssh
```

Additionally I commented out the "ALL" line in /etc/host.deny, and changed passwordAuth to yes in sshd_config
 

### Set Up Python (optional)
Python is a very useful tool for prototyping and quickly inspecting things, so let's go ahead and get it running with all some nice, standard scientific libraries.  One of the first things any python set up should get is to the package manager, pip.  Instructions for installing pip are found on the project's website, https://pip.pypa.io/en/stable/installing.html.  After installing pip we can install all the third-party libraries we want.

```bash
sudo pip install numpy scipy matplotlib scikit-learn
```

**Caveats:**
*  I had to install the python headers before anything else
```bash
sudo yum install python-devel
```
*  I still had to install a bunch of dependencies before getting the packages above to install with pip
```bash
sudo yum install libpng-devel qt-devel PyQt4 tkinter cmake lapack blas
```

### Installing the g-2 NMR-DAQ
Clone the code from the Redmine
```bash
git clone --recursive ssh://p-gm2daq@cdcvs.fnal.gov/cvs/projects/gm2daq
```

**Caveats:**
*  the "--recursive" option is to ask git to clone the code from the submodule as well as the container repository
*  if you forget it, or need to pull submodules into an already initialized repo use the following instead
```bash
git submodule update --init --recursive
```

#### nmr-core
The nmr-core submodule needs to be be built first, but that will probably require some dependencies first.  Some were simple, others not so simple.

##### g++
The first one is g++4.8 or newer to accommodate c++11 features.  To get this in Scientific Linux you need to install the devtoolset-2.  The solution to this was found on http://stackoverflow.com/questions/29790076/error-while-installing-gcc-4-8-on-scientific-linux-6.  I then added the tools to the path

```bash
wget -O /etc/yum.repos.d/slc6-devtoolset.repo http://linuxsoft.cern.ch/cern/devtoolset/slc6-devtoolset.repo
sudo yum install devtoolset-2
echo '$PATH=$PATH:/opt/rh/devtoolset-2/root/usr/bin' > ~/.bashrc
```

##### zmq
To get zmq I had  to add the EPEL package repo to install libsodium. Then the included repos for yum had a version of zmq, but it was too old (need version >= 4.0).  I cloned the repository, compiled zmq and installed it.

```bash
cd ~/Packages
sudo yum install libsodium-devel
git clone https://github.com/zeromq/libzmq
make && make install
```

##### boost
Once again, yum has a version of boost, but it is too old to accomodate all the code here.  We need to get a newer version.  I downloaded a tarball of the latest stable version and untared it into ~/Packages

```bash
cd ~/Packages/boost
./bootstrap.sh
./b2 install
```

##### fid-analysis
This is a small library that I wrote for simulating and analyzing FIDs.

```bash
cd ~/Packages
sudo yum install libfftw-devel libarmadillo-devel
git clone https://github.com/mwsmith2/fid-analysis
make && make install
```

##### include deps
Some of the worker classes depend on CAEN libraries, so we need to install as well.

```bash
sudo yum install libusb-1.0-devel wxGTK-devel
cd deps/CAENVMELib-2.41/lib
sudo ./install_64
cd ../../CAENComm-1.2/lib
sudo ./install_64
cd ../../CAENDigitizer_2.4
sudo ./install_64
```

#### nmr-core
Now we should be ready to compile nmr-core itself (including libuwlab.a).  Just before you install make the directory /usr/local/opt if it doesn't exist.

### NMR front-ends
This one should be easy by now.

```bash
sudo yum install sqlite-devel unixODBC-devel
make
```

If the front-ends don't compile let me know (mwsmith2@uw.edu), so I can fix this guide.

----------

## <a name="tutorial">Basic Tutorial</a>
Once you've gotten all the dependencies sorted out. We are ready to set up the experiment, and start taking some data.  One could look at the next [section](#layout) and set up the experiment layout, but the simplest thing to do is to clone the layout from the one I've set up on github.

```bash
cd ~/Applications
git clone https://github.com/uwmuonlab/gm2-nmr-daq gm2-nmr
```

Renaming it to gm2-nmr isn't strictly necessary, but I've gotten used to using that as the experiment base directory.  

### Configuring the Experiment

The next thing to do is set up the MIDAS exptab.  The exptab is located at `/etc/exptab`, and all that needs to be added is one line for the experiment.  It should be structured as `<expt-name> <expt-dir> <ext-user>`.  For example mine for gm2-nmr looks like

```bash
#/etc/exptab
gm2-nmr /home/newg2/Applications/gm2-nmr/resources newg2
simple-daq /home/newg2/Applications/simple-daq/resources newg2
```

The next thing to do is set up the experiment configuration in `common/.expt-env`.  This file is not tracked by the repository, so you'll need to copy the one from `common/examples/expt-env` to `common/.expt-env`. The file is a list of frontends and MIDAS utilities to run when the DAQ is started.  It's used by all the scripts in `online/bin/` which are scripts perform daq start/kill control.  Some of the variables here may need to be revised like the EXPT_IP to reflect the actual setup where things are being installed.

The ODB needs to be initialized as well.  I'd recommend making it a little bigger than the default, so that it doesn't become a problem later. Try the following command, `odbedit -e gm2-nmr -s 50000000`, to create a 50MB ODB.  After we create the ODB we need to configure some variables in the ODB.  The easiest way to do this is using the mhelper utility that I've been writing.  To install it do:

```bash
cd common/code/mhelper
sudo python setup.py install
```

With mhelper installed you can add all the ODB variables in a json file (see common/examples/add_to_odb.json) with `mhelper add-to-odb <path-to-json-file>`.

We also need to set up the data directory. I usually create the directory in /home/data/midas/<expt_name>/ then link it to the resource directory, `ln -s /home/data/midas/gm2-nmr ~/Applications/gm2-nmr/resources/data`.

Now let's build the frontends.

TODO - Uninstall gm2-nmr and reinstall from guide.


----------

## <a name="layout">Experiment Layout</a>
The DAQ structure is based on previous MIDAS experiments with a few tweaks to the in the mix.  One of the reasons for the restructure is to facilitate version control of the entire experiment directory.

### Folder Structure
I put these into `~/Applications/<expt-name>` but they can really go anywhere.
*  online
*  offline
*  common
*  resources

#### online
This folder is generally for utilities that are associated with the DAQ during the data taking process and run control. Mine contains the 3 folders
* bin - filled with start/stop scripts
* frontends - contains code + executable MIDAS front-ends
* www - contains html pages that will be linked to in the ODB

#### offline
This is for manipulation of data offline.  It hasn't been used much yet, so it's not a immature design.
* studies

#### common
The directory for things used by analyzers.
* .expt-env - important, experiment specific variables
* config - directory filled with config files used by the DAQ
* analyzers - used both online/offline so it lives here and gets symlinked
* scripts - utility scripts for things like resetting the ODB

#### resources
A directory containing all the binary data.  This is the only folder that does not go under version control (although git-lfs could change that for some of the data, like odb).  This directory also contains all the MIDAS SHM chunks.
* data
* elog
* history
* log
* backups

### Important Files

#### /etc/exptab
MIDAS needs to know about the experiment, so define it here with a line like
`<expt-name> <expt-dir> <expt-user>`
for instance my experiment current has
`gm2-nmr /home/newg2/Applications/gm2-nmr/resources newg2`
`simple-daq /home/newg2/Applications/simple-daq/resources newg2`


#### .expt-env
Contains lots of experiment specific environment variables, for instance:

```bash
#!/bin/bash
# Store some useful experiment specific variables that are used by
# scripts in one place

export EXPT='gm2-nmr'
export EXPT_IP='nmr-daq'
export EXPT_DIR='/home/newg2/Applications/gm2-nmr'
export MSERVER_PORT='7070'
export MHTTPD_PORT='8080'
export ROODY_PORT='9090'

export EXT_IP=(
)

export MIDAS_UTIL=(
    'mserver'
    'mhttpd'
    'mlogger'
    'mevb'
)

export EXPT_FE=(
    'shim_trigger'
    'shim_platform'
    'laser_tracker'
    'mscb_fe'
)

export EXPT_AN=(
)

# end script
```


