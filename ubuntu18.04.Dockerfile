# Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
#
#     http://aws.amazon.com/apache2.0/
#
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.

# Base image is Ubuntu 18.04
FROM ubuntu:18.04

# Update, install apt utils, curl, & unzip
RUN apt-get update \
&& apt-get install apt-utils build-essential curl unzip gcc g++ -y

# Install cmake
RUN apt-get install cmake -y

# Make APL Core
ADD . /apl-core
RUN cd apl-core \
	&& rm -rf build \
	&& mkdir build \
	&& cd build \
	&& cmake -DBUILD_TESTS=ON -DCOVERAGE=OFF .. \
	&& make -j4

 # RUN APL Core Tests
RUN cd apl-core/build \
	&& unit/unittest
