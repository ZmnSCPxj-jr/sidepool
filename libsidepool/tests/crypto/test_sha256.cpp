#if HAVE_CONFIG_H
# include"config.h"
#endif
#undef NDEBUG
#include"Sidepool/Crypto/SHA256.hpp"
#include<cassert>

int main() {
	typedef Sidepool::Crypto::SHA256
		SHA256;

	/* cat /dev/null | sha256sum */
	assert( SHA256::run("")
	     == SHA256::Hash("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")
	      );

	/* printf "hello" | sha256sum */
	assert( SHA256::run("hello")
	     == SHA256::Hash("2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824")
	      );

	/* 65 'i' characters.  */
	/* printf "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii" | sha256sum */
	assert( SHA256::run("iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii")
	     == SHA256::Hash("7db92d77e8b1d5ac593cd614244109b70618fe2a6a7eba541a5347ff383237d0")
	      );

	return 0;
}
