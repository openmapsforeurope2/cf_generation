
## Installation pour debian:
### Installation Docker
sudo apt-get update
sudo apt-get docker docker-compose

sudo groupadd docker
sudo usermod -aG docker ${USER}
sudo nano /etc/docker/daemon
+eventuellement: sudo chmod 666 /var/run/docker.sock

{ 
  "dns": ["172.21.2.14"],
  "dns-search": ["ign.fr"],
  "bip": "10.201.0.1/16",
  "registry-mirrors": [],
  "insecure-registries": ["dockerforge.ign.fr:5000","dockerforge.ign.fr","registry.ign-usine-logicielle.local.oshimae.rie.agri","registry.ul.geoportail.rie.g>
}

sudo systemctl restart docker


### creation image docker sd-externs-europe
cd /ome2/dev
git clone http://gitlab.forge-idi.ign.fr/socle/sd-externs.git ./sd-externs
cd sd-externs
git checkout ome2
cd docker
./build-docker-image-europe.sh


### creation image docker sd-socle-europe
cd /ome2/dev
git clone http://gitlab.forge-idi.ign.fr/socle/sd-socle.git ./sd-socle
cd sd-socle
git checkout ome2
cd docker
./build-docker-image-europe.sh

### libepg
### Création image docker libepg
cd /ome2/dev
git clone http://gitlab.dockerforge.ign.fr/europe/libepg.git ./libepg
cd libepg/docker
build-docker-image.sh

### Lancement des tests unitaires
cd /ome2/dev/libepg/docker
docker-compose up

### cf_generation
cd /ome2/dev
git clone https://github.com/openmapsforeurope2/cf_generation.git ./cf_generation
cd cf_generation/
./build-docker-image.sh

F1 > Tasks: Run Task > remove containers
F1 > Tasks: Run Task > build (in container)
F1 > Tasks: Run Task > prepare to debug

F5 (Run and Debug)
(gdb) Pipe Launch (cf_generation)