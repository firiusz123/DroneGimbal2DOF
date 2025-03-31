num = [8];
den = [1 , -0.01];
den1 = [1 , -0.2];
den2 = [1 , -1 ];


den_11 = conv(den,den1)
den_111 = conv(den_11,den2)


G = tf(num,den_111)
%%
rlocus(G)