#!/bin/bash
#==============================================================================#
# Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                    #
#                                                                              #
# This file is part of the OHWES Operating System.                             #
# OHWES is free software; you may redistribute it and/or modify it under the   #
# terms of the license agreement provided with this software.                  #
#                                                                              #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL      #
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER   #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER          #
# DEALINGS IN THE SOFTWARE.                                                    #
#==============================================================================#
#    File: scripts/devenv.sh                                                   #
# Created: November 27, 2020                                                   #
#  Author: Wes Hampson                                                         #
#                                                                              #
# Initializes the OHWES development environment.                               #
# To use, run `source devenv.sh` in your shell.                                #
#==============================================================================#

export OHWES_TOPDIR=$(dirname $(dirname $(realpath ${BASH_SOURCE[0]})))
export OHWES_BINDIR=${OHWES_TOPDIR}/bin
export OHWES_OBJDIR=${OHWES_TOPDIR}/obj
export OHWES_TOOLS=${OHWES_TOPDIR}/tools
export OHWES_SCRIPTS=${OHWES_TOPDIR}/scripts

export PATH=${OHWES_TOOLS}/bin:${OHWES_SCRIPTS}:${PATH}
