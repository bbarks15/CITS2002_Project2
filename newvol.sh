rm vol && ./sifs_mkvolume vol 1024 50 && ./sifs_mkdir vol a && ./sifs_mkdir vol a/b &&
    ./sifs_mkdir vol c && ./sifs_mkdir vol a/d && ./sifs_mkdir vol e &&
    ./sifs_mkdir vol a/1 && ./sifs_mkdir vol a/2 && ./sifs_mkdir vol a/3 &&
	./sifs_writefile vol newfile sifs.h && ./sifs_writefile vol newfile2 sifs.h &&
	./sifs_writefile vol a/newfile sifs.h && ./sifs_writefile vol a/1/README 1st.README
