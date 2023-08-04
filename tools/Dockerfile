FROM centos:8

LABEL maintainer="andrey.ivanov@ikfia.ysn.ru"
RUN dnf update -y
RUN dnf upgrade -y
RUN dnf install epel-release -y
EXPOSE 80
EXPOSE 4200
EXPOSE 4242
CMD ["ls", "-la", "ls;"]