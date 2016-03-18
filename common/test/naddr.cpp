#include <stdio.h>

#include <beyondy/xbs_naddr.h>

int main(int argc, char **argv)
{
	struct sockaddr addrs[100];
	socklen_t slens[100];

	for (int i = 1; i < argc; ++i) {
		int type = -1;
		int cnt = beyondy::str2sockaddr(argv[i], &type, addrs, slens, sizeof(addrs)/sizeof(addrs[0]));
		for (int j = 0; j < cnt; ++j) {
			char name[1024];
			beyondy::sockaddr2str(&addrs[j], type, name, sizeof name);
			printf("%s => %s\n", argv[i], name);
		}
	}
}
