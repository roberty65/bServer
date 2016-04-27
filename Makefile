
subdirs = common/src \
	  frame/share/src \
	  frame/main \
	  frame/testServer/src \
	  frame/testClient \
	  serverd/src \
	  sample/src

targets = all clean distclean
.PHONY: $(targets)

$(targets):
	@for dir in $(subdirs); do make -C $$dir $@; if test $$? -eq 2; then echo "!!!!! ERROR !!!!!"; exit; fi; done 
