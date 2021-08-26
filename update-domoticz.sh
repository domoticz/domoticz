# Set to domotics source directory
source=/home/efdeb/src/efdeb-domoticz

# Set to domotics runtime directory
target=/home/efdeb/domoticz

# Stop domoticz service
sudo service domoticz.sh stop

# Ensure backups and plugins directories exist
sudo mkdir -p $target/backups
sudo mkdir -p $target/plugins

# Copy domoticz executable
# One of the next two rsync comamnds will copy your domoticz binary to the target location
sudo rsync -I $source/bin/domoticz $target/domoticz # will error if the binary is not in $source/bin
sudo rsync -I $source/domoticz $target/domoticz     # will error if the binary is not in $source

# Sync domoticz config files
sudo rsync -rI $source/www/ $target/www
sudo rsync -rI $source/dzVents/ $target/dzVents
sudo rsync -rI $source/Config/ $target/Config
sudo rsync -rI $source/scripts/ $target/scripts

sudo rsync -I $source/History.txt $target
sudo rsync -I $source/License.txt $target
sudo rsync -I $source/server_cert.pem $target

