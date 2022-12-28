FROM eggeek01/gppc2021:gppc2021-base

# Dependicies Installation Example
RUN apt-get update
RUN apt-get --yes --no-install-recommends install clang

# Copy codes to target dir and set codes dir to be the working directory.
# Then run compile.sh to compile codes.
COPY ./. /GPPC2021/codes/
WORKDIR /GPPC2021/codes/
RUN chmod u+x compile.sh
RUN ./compile.sh
