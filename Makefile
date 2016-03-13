
subdirs = common/src \
	  frame/share/src \
	  frame/main \
	  frame/testServer/src \
	  frame/testClient \
	  serverd/src

targets = all clean distclean

$(targets):
	@for dir in $(subdirs); do if make -C $$dir $@; then : else exit; fi; done 
