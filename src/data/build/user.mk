INTERNAL_CFLAGS = -T$(LDSCRIPT) -znoexecstack \
				  -L$(LIBC_DIR) -I$(LIBC_DIR)/include/ -pie -no-pie
