{
	int s[3][1024];
	int m[3][4]; // min x,max x,min y,max y

	int miny = INT_MAX, maxy = INT_MIN;

	int x[3], y[3];
#if TEXTURE
	int u[3], v[3];
#endif
#if SMOOTH
	int r[3], g[3], b[3];
#endif
	int mii, mai;

	int xmin = DrawingArea[0];
	int xmax = DrawingArea[2];
	int ymin = DrawingArea[1];
	int ymax = DrawingArea[3];

	int rx = R_8(cv[0]);
	int gx = G_8(cv[0]);
	int bx = B_8(cv[0]);

	//int ptime, qtime;

	/*
#if TEXTURE && SMOOTH //GT3
	char *mstr = "GT3";
#else
#if !TEXTURE && SMOOTH //G3
	char *mstr = "G3";
#else
#if TEXTURE && !SMOOTH //FT3
	char *mstr = "FT3";
#else
#if !TEXTURE && !SMOOTH //F3
	char *mstr = "F3";
#endif
#endif
#endif
#endif*/

	//int qtime = clock();

	for (int i = 0; i < 3; i++) {
		if (yv[i] > maxy) {
			maxy = yv[i];
			mai = i;
		}
		if (yv[i] < miny) {
			miny = yv[i];
			mii = i;
		}
	}

	if (miny == maxy) return;

	if (maxy < ymin || miny > ymax) return;

	for (int i = 0; i < 3; i++) {
		if (i == mii) {
			x[0] = xv[i];
			y[0] = yv[i];

#if TEXTURE
			u[0] = uv[i];
			v[0] = vv[i]; 
#endif

#if SMOOTH
			r[0] = R_8(cv[i]);
			g[0] = G_8(cv[i]);
			b[0] = B_8(cv[i]);
#endif
		}
		else if (i == mai) {
			x[2] = xv[i];
			y[2] = yv[i];

#if TEXTURE
			u[2] = uv[i];
			v[2] = vv[i];
#endif

#if SMOOTH
			r[2] = R_8(cv[i]);
			g[2] = G_8(cv[i]);
			b[2] = B_8(cv[i]);
#endif
		}
		else {
			x[1] = xv[i];
			y[1] = yv[i];

#if TEXTURE
			u[1] = uv[i];
			v[1] = vv[i];
#endif

#if SMOOTH
			r[1] = R_8(cv[i]);
			g[1] = G_8(cv[i]);
			b[1] = B_8(cv[i]);
#endif
		}
	}

	for (int i = 0; i < 3; i++) {
		int x1 = x[i], y1 = y[i], x2 = x[(i + 1) % 3], y2 = y[(i + 1) % 3];
		m[i][0] = x1 < x2 ? x1 : x2;
		m[i][1] = x1 > x2 ? x1 : x2;
		m[i][2] = y1 < y2 ? y1 : y2;
		m[i][3] = y1 > y2 ? y1 : y2;
	}


	//ptime = clock();
	for (int i = 0; i < 3; i++) {
		arrayline(s[i], x[i], y[i], x[(i + 1) % 3], y[(i + 1) % 3]);
	}

	/*qtime = clock() - ptime;

	if (clock() - ptime > 10) {
		syslog(247, "!SLOW! %s %d,%d %d,%d %d,%d", mstr, xv[0], yv[0], xv[1], yv[1], xv[2], yv[2]);
		syslog(247, "!SLOW! arrayline %d ms", clock() - ptime);
	}*/

//#pragma omp parallel for
	//syslog(246, "yloop %d-%d", miny, maxy);

	if (miny < ymin) miny = ymin;
	if (maxy > ymax) maxy = ymax;

	for (int i = miny; i < maxy; i++) {
		int sa, sb;

		float tmp;
#if TEXTURE
		float ua, ub;
		float va, vb;
#endif

#if SMOOTH
		float ra, rb;
		float ga, gb;
		float ba, bb;
#endif

		if (i >= ymax || i < ymin) continue;
		if (i >= 512 || i < 0) continue;

		if (m[0][2] <= i && m[0][3] > i) {
			sa = s[0][i];

#if TEXTURE
			ua = INTERP(y[0], y[1], u[0], u[1], i);
			va = INTERP(y[0], y[1], v[0], v[1], i);
#endif

#if SMOOTH
			ra = INTERP(y[0], y[1], r[0], r[1], i);
			ga = INTERP(y[0], y[1], g[0], g[1], i);
			ba = INTERP(y[0], y[1], b[0], b[1], i);
#endif
		}
		if (m[1][2] <= i && m[1][3] > i) {
			sa = s[1][i];
			
#if TEXTURE
			ua = INTERP(y[1], y[2], u[1], u[2], i);
			va = INTERP(y[1], y[2], v[1], v[2], i);
#endif

#if SMOOTH
			ra = INTERP(y[1], y[2], r[1], r[2], i);
			ga = INTERP(y[1], y[2], g[1], g[2], i);
			ba = INTERP(y[1], y[2], b[1], b[2], i);
#endif
		}
		if (m[2][2] <= i && m[2][3] > i) {
			sb = s[2][i];

#if TEXTURE
			ub = INTERP(y[2], y[0], u[2], u[0], i);
			vb = INTERP(y[2], y[0], v[2], v[0], i);
#endif

#if SMOOTH
			rb = INTERP(y[2], y[0], r[2], r[0], i);
			gb = INTERP(y[2], y[0], g[2], g[0], i);
			bb = INTERP(y[2], y[0], b[2], b[0], i);
#endif
		}

		if (sa > sb) {
			tmp = sa;
			sa = sb;
			sb = tmp;

#if TEXTURE
			tmp = ua;
			ua = ub;
			ub = tmp;

			tmp = va;
			va = vb;
			vb = tmp;
#endif

#if SMOOTH
			tmp = ra;
			ra = rb;
			rb = tmp;

			tmp = ga;
			ga = gb;
			gb = tmp;

			tmp = ba;
			ba = bb;
			bb = tmp;
#endif
		}

		int minx = sa;
		int maxx = sb;

		if (sb < xmin || sa > xmax) continue;

		if (sa < xmin) sa = xmin;
		if (sb > xmax) sb = xmax;

		for (int j = sa; j < sb; j++) {
			if (j >= xmax || j < xmin) continue;
			if (j >= 1024 || j < 0) continue;

			Pixel = &((unsigned short *)fb)[i * 1024 + j];

#if TEXTURE
			int u = INTERP(minx, maxx, ua, ub, j);
			int v = INTERP(minx, maxx, va, vb, j);
#endif

#if SMOOTH
			int r = INTERP(minx, maxx, ra, rb, j);
			int g = INTERP(minx, maxx, ga, gb, j);
			int b = INTERP(minx, maxx, ba, bb, j);
#endif
#if TEXTURE && SMOOTH //GT3
			_LR = b;
			_LG = g;
			_LB = r;
			_TU = u;
			_TV = v;
#else
#if !TEXTURE && SMOOTH //G3
			PixelData = (0xF << 10) | (0xF << 5) | 0xF;
			_LR = b;
			_LG = g;
			_LB = r;
#else
#if TEXTURE && !SMOOTH //FT3
			PixelData = (0x1F << 10) | (0x1F << 5) | 0x1F | 0x8000;
			_TU = u;
			_TV = v;
			_LR = bx;
			_LG = gx;
			_LB = rx;
#else
#if !TEXTURE && !SMOOTH //F3
			int rgb = (rx << 16) | (gx << 8) | bx;
			PixelData = GPU_RGB16(rgb);
			_LR = bx;
			_LG = gx;
			_LB = rx;
#endif
#endif
#endif
#endif
			gpuDriver();
		}
	}

	//ptime = clock() - qtime;
	//if(ptime > 10) syslog(247, "full time %d", ptime);
}