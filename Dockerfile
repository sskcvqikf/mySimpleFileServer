FROM artixlinux/openrc

ARG WD=/usr/share/skqf_sss/

WORKDIR $WD

COPY ./main.cc ./Makefile ./config.hh ./config.mk ./entrypoint.sh $WD

RUN mkdir -p $WD/templates

COPY ./templates $WD/templates

RUN pacman --noconfirm -Syu make gcc boost zsh

RUN make

EXPOSE 6969

CMD ["./entrypoint.sh"]
