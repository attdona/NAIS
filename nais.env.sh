#!/bin/sh

export NAIS=$(pwd)

# pytest setup for cc3200
export PATH=$PATH:$NAIS/burba/boards/cc3200-launchxl/dist

VROOT=nais

conda_environment() {
    echo "creating virtual environment $VROOT"
    conda create --name $VROOT --yes
}

conda_activate() {
    source activate $VROOT
}

pip_environment() {
    python3 -m venv $VROOT
}

pip_activate() {
    . ${VROOT}/bin/activate
}

if type conda >/dev/null 2>&1 ; then
    echo "well done, setting up anaconda env"
    source activate $VROOT > /dev/null 2>&1
    if [ ! $? -eq 0 ]; then
        conda_environment
        source activate $VROOT
    fi    
elif [ ! -f ${VROOT}/bin/activate ] ; then
    echo "creating a standard virtual env"
    pip_environment
    pip_activate
else
    echo "activating a standard virtual env"
    pip_activate
fi


