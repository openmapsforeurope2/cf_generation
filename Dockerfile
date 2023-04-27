FROM libepg

RUN apt-get -y update
RUN apt-get -y install gdb

ENV APP_DIR_SRC=/usr/local/src/cf_generation/                                                     

WORKDIR $APP_DIR_SRC