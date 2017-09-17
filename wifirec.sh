#!/bin/sh

LENGTH=1
REF_LEVEL=-50

sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5180M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5200M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5220M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5240M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5260M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5280M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5300M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5320M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5500M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5520M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5540M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5560M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5580M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5660M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5680M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5700M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5720M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5745M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5765M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5785M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5805M && sync && \
sudo ./iqrecord -l $LENGTH -r $REF_LEVEL -c 5825M && sync

