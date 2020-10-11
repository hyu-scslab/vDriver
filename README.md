
# Prepare

1. Initiate submodules
```
git submodule init
git submodule update
```

2. Install packages.
```
sudo apt-get update  
sudo apt-get install -y gcc python3 python3-pip python-dateutil python3-dateutil bash sudo git gnuplot bison flex libreadline6 linux-tools-generic zlib1g-dev vim autoconf linux-tools-`uname -r` build-essential libreadline-dev pkg-config libmysqlclient-dev libpq-dev libtool
```

# PostgreSQL
```
cd PostgreSQL/Figure_12_the_CDF_of_version_chain_length
bash ./script_main.sh
```

# MySQL
```
cd mysql/script
python3 ./run_init.py ; python3 ./run_11.py
```



