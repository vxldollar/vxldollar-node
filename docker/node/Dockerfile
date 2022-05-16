FROM vxldollarcurrency/vxldollar-env:gcc

ARG NETWORK=live
ARG TRAVIS_TAG=DEV_BUILD
ARG CI_BUILD=OFF
ADD ./ /tmp/src

RUN mkdir /tmp/build && \
cd /tmp/build && \
cmake /tmp/src -DCI_BUILD=${CI_BUILD} \
-DBOOST_ROOT=${BOOST_ROOT} -DPORTABLE=1 \
-DACTIVE_NETWORK=vxldollar_${NETWORK}_network \
-DVXLDOLLAR_POW_SERVER=ON -DVXLDOLLAR_SHARED_BOOST=ON && \
make vxldollar_node -j $(nproc) && \
make vxldollar_rpc -j $(nproc) && \
make vxldollar_pow_server -j $(nproc) && \
cd .. && \
echo ${NETWORK} >/etc/vxldollar-network

FROM ubuntu:18.04

RUN groupadd --gid 1000 vxldollarcurrency && \
useradd --uid 1000 --gid vxldollarcurrency --shell /bin/bash --create-home vxldollarcurrency

COPY --from=0 /tmp/build/vxldollar_node /usr/bin
COPY --from=0 /tmp/build/vxldollar_rpc /usr/bin
COPY --from=0 /tmp/build/vxldollar_pow_server /usr/bin
COPY --from=0 /tmp/src/api/ /usr/bin/api/
COPY --from=0 /etc/vxldollar-network /etc
COPY docker/node/entry.sh /usr/bin/entry.sh
COPY docker/node/config /usr/share/vxldollar/config
COPY --from=0 /tmp/boost/lib/* /usr/local/lib/
RUN chmod +x /usr/bin/entry.sh
RUN ln -s /usr/bin/vxldollar_node /usr/bin/rai_node
RUN ldconfig

WORKDIR /root
USER root

ENV PATH="${PATH}:/usr/bin"
ENTRYPOINT ["/usr/bin/entry.sh"]
CMD ["vxldollar_node", "daemon", "-l"]
ARG REPOSITORY=vxldollarcurrency/vxldollar-node
LABEL org.opencontainers.image.source https://github.com/$REPOSITORY
