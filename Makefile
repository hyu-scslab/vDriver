


IMAGE=vdriver_image
CONTAINER=vdriver_container


init:
	git submodule init
	git submodule update


#build_docker_image:
#	sudo docker rmi vdriver_image;\
#		sudo docker build -t vdriver_image .

build_docker_image:
	sudo docker build -t vdriver_image .

run_docker_container:
	sudo docker rm -f vdriver_container;\
		sudo docker run -i -t --rm --name vdriver_container vdriver_image


kill_docker_container:
	sudo docker rm -f `sudo docker ps -a -q`


rm_docker_image:
	sudo docker rmi vdriver_image




figure_03:
	bash ./PostgreSQL/Figure_03_effects_of_a_llt/script_main.sh









