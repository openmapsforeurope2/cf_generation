FROM libepg

ARG GIT_BRANCH
ARG NB_PROC

ENV APP_DIR_SRC=/usr/local/src/cf_generation/                                                     

COPY . $APP_DIR_SRC

WORKDIR $APP_DIR_SRC

RUN git checkout $GIT_BRANCH

RUN mkdir build-unix

WORKDIR $APP_DIR_SRC/build-unix

RUN cmake -DCMAKE_INSTALL_PREFIX=$APP_DIR_SRC .. && make -j$NB_PROC install

WORKDIR $APP_DIR_SRC
RUN rm -r $APP_DIR_SRC/build-unix

ENV PATH=$PATH:$APP_DIR_SRC/bin