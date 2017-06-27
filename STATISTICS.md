![NexMon logo](https://github.com/seemoo-lab/nexmon/raw/master/gfx/nexmon.png)

# What kind of statistics do you collect?

Everytime you run a Nexmon firmware build, we collect the following information:
* A unique identifier based on a random number (e.g., 5O31UY9Z5IEX3O9KL680V5IHNASIE1SB)
* The name, release, machine and processor of your build system (`uname -srmp`, e.g., `Linux 4.2.0-42-generic x86_64 x86_64`)
* Git internal path to the built project (e.g., `patches/bcm4339/6_37_34_43/nexmon/`)
* Git version (e.g., `2.2.1-55-g3684a80c-dirty`)
* Git repository URL (e.g., `git@github.com:seemoo-lab/wisec2017_nexmon_jammer.git`)

# Why do you collect statistics?

Nexmon is mainly intended as a research project that we share with the community so that others can benefit from our tools.
We want to collect statistics to figure out how often Nexmon is used in general and which platform and firmware version is the most popular.
We also intent to share our findings in the future.

# How do I disable the collection of statistics?

If you have privacy concerns, we also offer to opt-out of the statistic collections. To this end, you simply have to create a `DISABLE_STATISTICS` file in your Nexmon root directory.
