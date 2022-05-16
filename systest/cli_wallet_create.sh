#!/bin/sh

set -e

DATADIR=data.systest

SEED=CEEDCEEDCEEDCEEDCEEDCEEDCEEDCEEDCEEDCEEDCEEDCEEDCEEDCEEDCEEDCEED

# the caller should set the env var VXLDOLLAR_NODE_EXE to point to the vxldollar_node executable
# if VXLDOLLAR_NODE_EXE is unser ot empty then "../../build/vxldollar_node" is used
VXLDOLLAR_NODE_EXE=${VXLDOLLAR_NODE_EXE:-../../build/vxldollar_node}

clean_data_dir() {
    rm -f  $DATADIR/log/log_*.log
    rm -f  $DATADIR/wallets.ldb*
    rm -f  $DATADIR/data.ldb*
    rm -f  $DATADIR/config-*.toml
    rm -rf "$DATADIR"/rocksdb/
}

mkdir -p $DATADIR/log
clean_data_dir

# initialise data directory
$VXLDOLLAR_NODE_EXE --initialize --data_path $DATADIR

# create a wallet and store the wallet ID
wallet_id=`$VXLDOLLAR_NODE_EXE --wallet_create --data_path $DATADIR --seed $SEED`

# decrypt the wallet and check the seed
$VXLDOLLAR_NODE_EXE --wallet_decrypt_unsafe --wallet $wallet_id --data_path $DATADIR | grep -q "Seed: $SEED"

# list the wallet and check the wallet ID
$VXLDOLLAR_NODE_EXE --wallet_list --data_path $DATADIR | grep -q "Wallet ID: $wallet_id"

# if it got this far then it is a pass
echo $0: PASSED
exit 0
