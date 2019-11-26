{
	int m, anum, bnum, oldm;
	int maxy, miny;
	int x1A, x2A;
	int x1B, x2B;
#if TEXTURE
	int u1A, u2A, v1A, v2A;
	int u1B, u2B, v1B, v2B;
#endif
#if SMOOTH
	int r1A, r2A, g1A, g2A, b1A, b2A;
	int r1B, r2B, g1B, g2B, b1B, b2B;
#endif
	int fy;
	int i, x, y;
	int sy, ey;

	a3d_liner y8A, y8B;
	a3d_liner x8;

	a3d_vertex *lpVer;

	int xx1, xx2;
	int tmp;

#if TEXTURE
	int uu1, uu2;
	int vv1, vv2;
#endif

#if SMOOTH
	int rr1, rr2;
	int gg1, gg2;
	int bb1, bb2;
#endif

	int xmin = DrawingArea[0];
	int xmax = DrawingArea[2];
	int ymin = DrawingArea[1];
	int ymax = DrawingArea[3];

#if !SMOOTH
	int rx, gx, bx;
	rx = R_8(v[0].c);
	gx = G_8(v[0].c);
	bx = B_8(v[0].c);
#endif


	//Y座標値で最大・最小のものを求める
	m = 0;
	lpVer = v;
	maxy = miny = lpVer->y;
	lpVer++;
	for (i = 1; i<VERTEX; i++) {
		fy = lpVer->y;
		if (miny>fy) {
			m = i;
			miny = fy;
		}
		if (maxy<fy) maxy = fy;
		lpVer++;
	}
	sy = miny;
	ey = maxy;
	if (sy == ey) return;

	//左側にある頂点とX方向増加値の計算
	anum = bnum = m;
	do {
		anum++;
		if (anum>VERTEX - 1) anum = 0;
	} while (v[anum].y<sy);
	m = anum - 1; if (m<0) m = VERTEX - 1;

	a3d_lninit(&y8A, v[m].y, v[anum].y);

	x1A= v[m].x, x2A = v[anum].x;
#if TEXTURE
	u1A = v[m].u, u2A = v[anum].u;
	v1A = v[m].v, v2A = v[anum].v;
#endif

#if SMOOTH
	r2A = R_8(v[anum].c), r1A = R_8(v[m].c);
	g2A = G_8(v[anum].c), g1A = G_8(v[m].c);
	b2A = B_8(v[anum].c), b1A = B_8(v[m].c);
#endif

	//右側にある頂点とX方向増加値の計算
	do {
		bnum--;
		if (bnum<0) bnum = VERTEX - 1;
	} while (v[bnum].y<sy);
	m = bnum + 1; if (m>VERTEX - 1) m = 0;

	a3d_lninit(&y8B, v[m].y, v[bnum].y);

	x1B = v[m].x, x2B = v[bnum].x;
#if TEXTURE
	u1B = v[m].u, u2B = v[bnum].u;
	v1B = v[m].v, v2B = v[bnum].v;
#endif

#if SMOOTH
	r2B = R_8(v[bnum].c), r1B = R_8(v[m].c);
	g2B = G_8(v[bnum].c), g1B = G_8(v[m].c);
	b2B = B_8(v[bnum].c), b1B = B_8(v[m].c);
#endif

	DISABLE_CHECK

	y = sy;
	while(1) {
		//y++;
		if (y >= ymax) break;

		//左側稜線の処理
		if (v[anum].y<y+1) {
			do {
				anum++;
				if (anum>VERTEX - 1) anum = 0;
			} while (v[anum].y<y);
			oldm = anum - 1; if (oldm<0) oldm = VERTEX - 1;

			a3d_lninit(&y8A, v[oldm].y, v[anum].y);

			//y8A.dx++;

			x1A = v[oldm].x, x2A = v[anum].x;
#if TEXTURE
			u1A = v[oldm].u, u2A = v[anum].u;
			v1A = v[oldm].v, v2A = v[anum].v;
#endif

#if SMOOTH
			r2A = R_8(v[anum].c), r1A = R_8(v[oldm].c);
			g2A = G_8(v[anum].c), g1A = G_8(v[oldm].c);
			b2A = B_8(v[anum].c), b1A = B_8(v[oldm].c);
#endif
		}
		//右稜線の処理
		if (v[bnum].y<y+1) {
			do {
				bnum--;
				if (bnum<0) bnum = VERTEX - 1;
			} while (v[bnum].y<y);
			oldm = bnum + 1; if (oldm>VERTEX - 1) oldm = 0;

			a3d_lninit(&y8B, v[oldm].y, v[bnum].y);

			//y8B.dx++;

			x1B = v[oldm].x, x2B = v[bnum].x;
#if TEXTURE
			u1B = v[oldm].u, u2B = v[bnum].u;
			v1B = v[oldm].v, v2B = v[bnum].v;
#endif

#if SMOOTH
			r2B = R_8(v[bnum].c), r1B = R_8(v[oldm].c);
			g2B = G_8(v[bnum].c), g1B = G_8(v[oldm].c);
			b2B = B_8(v[bnum].c), b1B = B_8(v[oldm].c);
#endif
		}
		if (y >= ymin && y < ymax) {
			//X軸方向の描画
			int fixA = y8A.y0;
			int fixB = y8B.y0;

			xx1 = FIX_MIX_NM(x1A, x2A, fixA);
			xx2 = FIX_MIX_NM(x1B, x2B, fixB);
#if TEXTURE
			uu1 = FIX_MIX_EX(u1A, u2A, fixA);
			uu2 = FIX_MIX_EX(u1B, u2B, fixB);
			vv1 = FIX_MIX_EX(v1A, v2A, fixA);
			vv2 = FIX_MIX_EX(v1B, v2B, fixB);
#endif

#if SMOOTH
			rr1 = FIX_MIX_EX(r1A, r2A, fixA);
			rr2 = FIX_MIX_EX(r1B, r2B, fixB);
			gg1 = FIX_MIX_EX(g1A, g2A, fixA);
			gg2 = FIX_MIX_EX(g1B, g2B, fixB);
			bb1 = FIX_MIX_EX(b1A, b2A, fixA);
			bb2 = FIX_MIX_EX(b1B, b2B, fixB);
#endif

			if (xx2<xx1) {
				tmp = xx1; xx1 = xx2; xx2 = tmp;
#if TEXTURE
				tmp = uu1; uu1 = uu2; uu2 = tmp;
				tmp = vv1; vv1 = vv2; vv2 = tmp;
#endif

#if SMOOTH
				tmp = rr1; rr1 = rr2; rr2 = tmp;
				tmp = gg1; gg1 = gg2; gg2 = tmp;
				tmp = bb1; bb1 = bb2; bb2 = tmp;
#endif
			}

			int xu, xv, xr, xg, xb;

			a3d_lninit(&x8, xx1, xx2);

			//１ライン分を描画
			x = xx1;
			while(1) {
				if (x >= xmin && x < xmax) {
					Pixel = &((unsigned short *)dst.pixel)[y * dst.w + x];

					int fix = x8.y0;
#if TEXTURE
					xu = FIX_MIX(uu1, uu2, fix);
					xv = FIX_MIX(vv1, vv2, fix);
#endif
#if SMOOTH
					xr = FIX_MIX(rr1, rr2, fix);
					xg = FIX_MIX(gg1, gg2, fix);
					xb = FIX_MIX(bb1, bb2, fix);
#endif
#if TEXTURE && SMOOTH //GT3
					//PixelShader(xu, xv, xr, xg, xb);
					_LR = xb;
					_LG = xg;
					_LB = xr;
					_TU = xu;
					_TV = xv;
#else
#if !TEXTURE && SMOOTH //G3
					//PixelShader(0, 0, xr, xg, xb);
					PixelData = (0xF << 10) | (0xF << 5) | 0xF;
					_LR = xb;
					_LG = xg;
					_LB = xr;
#else
#if TEXTURE && !SMOOTH //FT3
					//PixelShader(xu, xv, rx, gx, bx);
					PixelData = (0x1F << 10) | (0x1F << 5) | 0x1F | 0x8000;
					_TU = xu;
					_TV = xv;
					_LR = bx;
					_LG = gx;
					_LB = rx;
#else
#if !TEXTURE && !SMOOTH //F3
					//PixelShader(0, 0, rx, gx, bx);
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
				x++;
				if (x >= xx2) break;
				a3d_lnstep(&x8);
			}
		}
		//y--;
		y++;
		if (y >= ey) break;
		a3d_lnstep(&y8A);
		a3d_lnstep(&y8B);
	}

	ENABLE_CHECK
}