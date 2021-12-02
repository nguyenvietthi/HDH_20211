cmd_/home/thinv/HDH_20211/Module.symvers := sed 's/ko$$/o/' /home/thinv/HDH_20211/modules.order | scripts/mod/modpost -m -a   -o /home/thinv/HDH_20211/Module.symvers -e -i Module.symvers   -T -
