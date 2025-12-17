# To Build: docker build --no-cache -t pbui/cse-30341-fa25-assignments . 

FROM        ubuntu:24.04
LABEL	    org.opencontainers.image.authors="Peter Bui <pbui@nd.edu>"
ENV	    DEBIAN_FRONTEND=noninteractive

# Update package metadata
RUN	    apt-get update -y

# Run-time dependencies
RUN	    apt-get install -y python3-tornado python3-requests python3-yaml python3-markdown

# Shell utilities
RUN	    apt-get install -y curl bc netcat-openbsd iproute2 zip unzip gawk

# Language Support: C, C++, Make, valgrind, cppcheck
RUN	    apt-get install -y gcc g++ make valgrind cppcheck libssl-dev
