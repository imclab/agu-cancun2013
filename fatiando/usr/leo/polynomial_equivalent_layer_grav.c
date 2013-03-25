#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define GRAV_CONSTANT 66.7E-12 /* (meter^3)/(kilogram*second^2) */ 
#define MSTOMGAL 1E5 /* converts meter per squared second to miliGal */

/* Fun��es retiradas do Numerical Recipes, utilizadas para contaminar os dados com r�ido
aleat�rio e com distribui��o Gaussiana ==> */

	/* vari�vel utilizada nas fun��es gasdev e ran1, abaixo */

	long idum;

	/* Fun��o gasdev do cap�tulo sete (Random Numbers) do Numerical Recipes */

	/* Returns a normally distributed deviate with zero mean and unit variance, using ran1(idum)
	as the source of uniform deviates. */

	float gasdev(long *);

	/* Fun��o ran1 do cap�tulo sete (Random Numbers) do Numerical Recipes */

	/* "Minimal" random number generator of Park and Miller with Bays-Durham shuffle and added
	safeguards. Returns a uniform random deviate between 0.0 and 1.0 (exclusive of the endpoint
	values). Call with idum a negative integer to initialize; thereafter, do not alter idum between
	successive deviates in a sequence. RNMX should approximate the largest floating value that is
	less than 1. */

	#define IA 16807
	#define IM 2147483647
	#define AM (1.0/IM)
	#define IQ 127773
	#define IR 2836
	#define NTAB 32
	#define NDIV (1+(IM-1)/NTAB)
	#define EPS 1.2e-7
	#define RNMX (1.0-EPS)

	float ran1(long *);

/* <== Fun��es retiradas do Numerical Recipes, utilizadas para contaminar os dados com r�ido
aleat�rio e com distribui��o Gaussiana */


double **aloca_matriz_double (FILE *, int, int);

double **libera_matriz_double (int, double **);

double *aloca_vetor_double (FILE *, int);

double *libera_vetor_double (double *);

double sist_lin_LU (FILE *relatorio, int M, double **GtG, double *Gtd, double *p, double uridge);

void choldc(FILE *relatorio, double **a, int n, double *p);

void cholsl(double **a, int n, double *p, double *b, double *x);

void leitura_parametros (FILE *relatorio, int *N, double *uridge, double *usuavidade, double *Z_layer, int *grau, double *stdev_dados, int *semente, int *Npolx, int *Npoly, int *Nspolx, int *Nspoly, double *lambidax, double *lambiday);

void leitura_dados(FILE *relatorio, int N, double *Xp, double *Yp, double *Zp, double *gobs, double *xmin, double *xmax, double *ymin, double *ymax, double stdev_dados);

void coordenadas_fontes(int Npolx, int Npoly, int Nspolx, int Nspoly, double dx, double dy, double X1, double Y1, double *X, double *Y);

void calcula_BtAtAB (int N, int Q, int Npol, int Nspol, int grau, int M, int H, double *Y, double *X, double Z_layer, double *yp, double *xp, double *zp, double *a, double **B, double *g, double **GtG, double *Gtgzobs, double *gzobs, double *fator_normalizacao);

void calcula_B (int Npol, int Nspol, int grau, double *Y, double *X, double **B, double Y1, double Y2, double X1, double X2);

void calcula_BtRtRB (int H, int Q, int Npolx, int Npoly, int Nspol, int Nsy, int Nspolx, int Nspoly, double **B, double *g, double **GtG, double fator_normalizacao, double usuavidade);

void incorpora_ridge (int H, double **GtG, double uridge, double fator_normalizacao);

void calcula_densidades(int M, int H, double *c, double *p, double **B);

void linha_a (int i, int M, double *Y, double *X, double Z_layer, double *yp, double *xp, double *zp, double *a);

void gz_equivalente(int N, int M, double *yp, double *xp, double *zp, double *Y, double *X, double Z_layer, double *p, double *gz);

double gz_monopolo(int i, double *yp, double *xp, double *zp, int j, double *Y, double *X, double Z_layer);

void impressao_componente(FILE *arquivo, int N, double *y, double *x, double *z, double *gzobs, double *componente);

void impressao_camada(FILE *arquivo, int M, double *Y, double *X, double *p);

void gzz_equivalente(int N, int M, double *yp, double *xp, double *zp, double *Y, double *X, double Z_layer, double *p, double *gzz);

double gzz_monopolo(int i, double *yp, double *xp, double *zp, int j, double *Y, double *X, double Z_layer);

void main () {

	int N, M, Npol, Nspol, Npoly, Npolx, Nsy, Nsx, Nspoly, Nspolx, grau, Q, H;
	int i, j, k;
	int semente;
	double *gzobs, *xp, *yp, *zp, xmin, xmax, ymin, ymax;
	double Y1, X1, Y2, X2, lambiday, lambidax, dy, dx;
	double *gz, *X, *Y, Z_layer, *c, *p;
	double *a, **B, *g;
	double aux0, aux1, aux2;
	double uridge, usuavidade, fator_normalizacao, **BtRtRB, **BtAtAB, **GtG, *Gtgzobs;
	double stdev_dados;
	char str[100];

	time_t start1, end1, start2, end2;

	FILE *entrada, *saida, *relatorio;
	
	time (&start1);

	/*

	Sistema de coordenadas
	**********************
	x aponta para norte (metros)
	y aponta para leste (metros)
	z aponta para baixo (metros)

	Descri��o das vari�veis
	***********************
	N = N�mero de dados observados.
	M = N�mero total de fontes equivalentes.
	Nsx = N�mero total de fontes equivalentes na dire��o x.
	Nsy = N�mero total de fontes equivalentes na dire��o y.
	Npolx = N�mero de polin�mios na dire��o x.
	Npoly = N�mero de polin�mios na dire��o y.
	Nspolx = N�mero de fontes equivalentes, na dire��o x, em cada polin�mio.
	Nspoly = N�mero de fontes equivalentes, na dire��o y, em cada polin�mio.
	Nspol = N�mero de fontes equivalentes em cada polin�mio.
	dx = Dist�ncia entre as fontes equivalentes na dire��o x.
	dy = Dist�ncia entre as fontes equivalentes na dire��o y.
	gzobs = Vetor que armazena gz observado.
	xp = Vetor que armazena as coordenadas x do gz observado.
	yp = Vetor que armazena as coordenadas y do gz observado.
	zp = Vetor que armazena as coordenadas z do gz observado.
	xmin = Coordenada mais a sul do gz.
	xmax = Coordenada mais a norte do gz.
	ymin = Coordenada mais a oeste do gz.
	ymax = Coordenada mais a leste do gz.
	X1 = Coordenada mais a sul da camada equivalente.
	X2 = Coordenada mais a norte da camada equivalente.
	Y1 = Coordenada mais a oeste da camada equivalente.
	Y2 = Coordenada mais a leste da camada equivalente.
	lambidax = extens�o da camada equivalente na dire��o x para al�m dos limites dos dados observados.
	lambiday = extens�o da camada equivalente na dire��o y para al�m dos limites dos dados observados.
	gz = Vetor que armazena gz predito pela camada equivalente. 
	X = vetor que armazena as coordenadas X das fontes equivalentes.
	Y = vetor que armazena as coordenadas Y das fontes equivalentes.
	Z_layer = coordenada Z_layer das fontes equivalentes.
	c = vetor que armazena os coeficientes dos polin�mios que descrevem as densidades das fontes equivalentes.
	grau = Vari�vel que armazena o grau dos polinomios que descrevem as densidades das fontes equivalentes.
	Q = N�mero de coeficientes de cada um dos polinomios que descrevem as densidades das fontes equivalentes.
	H = N�mero total de coeficientes dos polinomios que descrevem as densidades das fontes equivalentes.
	uridge = par�metro de regulariza��o Ridge Regression (Tikhonov de ordem zero) para o c�lculo da distribui��o
		de densidades na camada equivalente.
	usuavidade = par�metro de regulariza��o Suavidade (Tikhonov de ordem um) para o c�lculo da distribui��o
		de densidades na camada equivalente.

	Descri��o breve do programa
	***************************

	Seja um vetor N-dimensional de dados observados (gravidade) gzobs, � poss�vel calcular uma
	camada equivalente que ajusta esses dados. Essa camada � formada por monopolos com densidades
	diferentes. O c�lculo da distribui��o de densidades nos monopolos � feita resolvendo-se o
	seguinte sistema linear:

	Ap = gzobs,												(1)

	sendo p um vetor M-dimensional com as densidades dos monopolos e A uma matriz N x M, cujo
	elemento ij representa o efeito gravitacional, na posi��o xp[i], yp[i] e zp[i], de um monopolo
	com densidade unit�ria e localizado na posi��o X[j], Y[j] e Z_layer.
	
	A camada equivalente pode ser dividida regularmente nas dire��es x e y, de forma que a distribui��o
	de densidades dentro de cada setor retangular seja descrita por um polin�mio de grau "grau". Essa
	distribui��o de densidades polinomial pode ser representada da seguinte forma:

	Bc = p,													(2)

	em que c � um vetor H-dimensional, cujos elementos s�o os coeficientes dos polin�mios que descrevem
	a distribui��o de densidades na camada equivalente e B � uma matriz M x H, cujo elemento ij � a
	derivada do conjunto de polin�mios, na posi��o X[i], Y[i] e Z_layer, em rela��o ao j-�simo coeficiente.

	Como a camada � dividida em setores retangulares e cada setor tem uma distribui��o de densidades
	descrita por um polin�mio, a derivada em rela��o a um coeficiente j de um polin�mio que pertence
	a um setor k, calculada em uma posi��o X[j], Y[j] e Z_layer pertencente qualquer setor l diferente de k
	possui valor nulo. Essa derivada n�o � nula apenas quando calculada em uma posi��o X[j], Y[j] e Z_layer
	pentencento ao mesmo setor k do polin�mio.

	Substituindo 2 em 1 chega-se ao seguinte sistema linear

	ABc = gzobs												(3)

	e o problema de estimar a densidade em cada monopolo da camada equivalente passa a ser estimar
	os valores dos coeficientes que descrevem a distribui��o de densidades em cada setor da camada
	equivalente. Esse procedimento pode diminuir dr�sticamente o esfor�o computacional envolvido no
	c�lculo da camada equivalente, visto que � poss�vel descrever uma grande quantidade de monopolos
	com um polin�mio de grau baixo.

	Para que seja poss�vel, por exemplo, fazer continua��o para cima ou calcular o tensor de gravidade
	por meio da camada equivalente, � necess�rio que a distribui��o de densidades estimada seja suave.
	Sendo assim, essa informa��o foi imposta por meio do regularizador de Tikhonov de ordem um

	Rp = 0,													(4)

	sendo R uma matriz de diferen�as finitas, e o problema de estimar a distribui��o de densidades na
	camada equivalente passa a ser estimar o vetor c que minimiza a express�o

	(ABc - gzobs)t(ABc - gzobs) + u(RBc)t(RBc)				(5)

	sendo u	o par�metro de regulariza��o. A solu��o da express�o 5 � dada em termos da solu��o do
	seguinte sistema linear

	(BtAtAB + uBtRtRB)c = BtAtgzobs.						(6)

	*/

	relatorio = fopen("relatorio.txt", "w");

	leitura_parametros (relatorio, &N, &uridge, &usuavidade, &Z_layer, &grau, &stdev_dados, &semente, &Npolx, &Npoly, &Nspolx, &Nspoly, &lambidax, &lambiday);

	xp = aloca_vetor_double(relatorio, N);
	yp = aloca_vetor_double(relatorio, N);
	zp = aloca_vetor_double(relatorio, N);
	gzobs = aloca_vetor_double(relatorio, N);

	leitura_dados(relatorio, N, xp, yp, zp, gzobs, &xmin, &xmax, &ymin, &ymax, stdev_dados);

	/* C�lculo dos limites da camada equivalente ==> */
	X1 = xmin - (lambidax*(xmax - xmin));
	X2 = xmax + (lambidax*(xmax - xmin));
	Y1 = ymin - (lambiday*(ymax - ymin));
	Y2 = ymax + (lambiday*(ymax - ymin));
	/* <== C�lculo dos limites da camada equivalente */

	/* C�lculo das coordenadas X e Y das fontes equivalentes ==> */
	printf ("calculo das coordenadas das fontes\n\n");

	Npol = Npolx*Npoly;

	Nsx = Npolx*Nspolx;
	Nsy = Npoly*Nspoly;

	Nspol = Nspolx*Nspoly;
	
	dx = (double)((X2 - X1)/(Nsx - 1.0));
	dy = (double)((Y2 - Y1)/(Nsy - 1.0));
	
	M = Nsx*Nsy;

	/* C�lculo do n�mero de coeficientes dos polin�mios ==> */
	for (Q = 1, i = 2; i <= (grau+1); i++) {
	
		Q += i; 
	
	}
	/* <== C�lculo do n�mero de coeficientes dos polin�mios */

	H = Npol*Q;

	X = aloca_vetor_double(relatorio, M);
	Y = aloca_vetor_double(relatorio, M);

	coordenadas_fontes(Npolx, Npoly, Nspolx, Nspoly, dx, dy, X1, Y1, X, Y);
	/* <== C�lculo das coordenadas X e Y das fontes equivalentes */

	/* C�lculo da matriz B ==> */
	time (&start2);
	
	B = aloca_matriz_double (relatorio, M, H);

	calcula_B (Npol, Nspol, grau, Y, X, B, Y1, Y2, X1, X2);
	
	time (&end2);
	
	aux0 = difftime (end2, start2);
	
	fprintf (relatorio, "C�lculo da matriz B (%.5lf segundos)\n\n", aux0);
	
	/* <== C�lculo da matriz B */

	GtG = aloca_matriz_double (relatorio, H, H);
	BtRtRB = aloca_matriz_double (relatorio, H, H);
	BtAtAB = aloca_matriz_double (relatorio, H, H);
	Gtgzobs = aloca_vetor_double (relatorio, H);
	g = aloca_vetor_double (relatorio, H);

	/* C�lculo da matriz GtG = BtAtAB e do vetor Gtgzobs ==> */
	time (&start2);
	
	a = aloca_vetor_double (relatorio, M);

	printf ("matriz BtAtAB\n\n");
	calcula_BtAtAB (N, Q, Npol, Nspol, grau, M, H, Y, X, Z_layer, yp, xp, zp, a, B, g, BtAtAB, Gtgzobs, gzobs, &fator_normalizacao);

	time (&end2);
	
	aux0 = difftime (end2, start2);
	
	fprintf (relatorio, "C�lculo da matriz BtAtAB (%.5lf segundos)\n\n", aux0);

	/* <== C�lculo da matriz GtG = BtAtAB e do vetor Gtgzobs */

	/* Incorpora��o do v�nculo de suavidade. C�lculo da matriz BtRtRB ==> */
	time (&start2);
	
	printf ("matriz BtRtRB\n\n");

	calcula_BtRtRB (H, Q, Npolx, Npoly, Nspol, Nsy, Nspolx, Nspoly, B, g, BtRtRB, fator_normalizacao, usuavidade);
	
	time (&end2);
	
	aux0 = difftime (end2, start2);
	
	fprintf (relatorio, "C�lculo da matriz BtRtRB (%.5lf segundos)\n\n", aux0);
	
	/* <== Incorpora��o do v�nculo de suavidade. C�lculo da matriz BtRtRB */
	
	/* Calcula GtG = (BtRtRB + BtAtAB) ==> */
	for (i = 0; i < H; i++) {
	
		for (j = i; j < H; j++) {
		
			GtG[i][j] = (BtRtRB[i][j] + BtAtAB[i][j]);
		
		}
	
	}
	/* <== Calcula GtG = (BtRtRB + BtAtAB) */

	/* incorpora��o do v�nculo Ridge */
	incorpora_ridge (H, GtG, uridge, fator_normalizacao);
	
	/* Estimativa dos coeficientes dos polin�mios que descrevem as densidades 
	na camada equivalente ==> */
	c = aloca_vetor_double (relatorio, H);

	time (&start2);
	
	printf ("\nResolucao do sistema linear\n");
	/* Resolu��o do sistema GtGc = Gtgzobs */
	/*sist_lin_LU (relatorio, H, GtG, Gtgzobs, c, 0.0);*/
	
	choldc(relatorio, GtG, H, g); /* Decomposi��o de Cholesky */
	
	cholsl(GtG, H, g, Gtgzobs, c); /* Resolu��o do sistema linear */

	time (&end2);

	aux0 = difftime (end2, start2);
	
	fprintf (relatorio, "Resolucao do sistema linear (%.5lf segundos)\n\n", aux0);

	GtG = libera_matriz_double(H, GtG);
	BtRtRB = libera_matriz_double(H, BtRtRB);
	BtAtAB = libera_matriz_double(H, BtAtAB);
	Gtgzobs = libera_vetor_double(Gtgzobs);
	g = libera_vetor_double (g);
	/* <== Estimativa dos coeficientes dos polin�mios que descrevem as densidades 
	na camada equivalente */

	/* C�lculo das densidades na camada equivalente ==> */
	printf ("\nCalculo das densidades da camada\n");
	p = aloca_vetor_double (relatorio, M);
	
	calcula_densidades(M, H, c, p, B);
	
	B = libera_matriz_double (M, B);
	/* <== C�lculo das densidades na camada equivalente */

	/* C�lculo de gz e das componentes do tensor ==> */
	printf ("\nCalculo de gz\n\n");
	
	gz = aloca_vetor_double (relatorio, N);

	gz_equivalente(N, M, yp, xp, zp, Y, X, Z_layer, p, gz);

	a = libera_vetor_double (a);
	/* <== C�lculo de gz e das componentes do tensor */

	/* impress�o dos arquivos de saida ==> */

	saida = fopen ("ajuste.txt", "w");
	impressao_componente (saida, N, yp, xp, zp, gzobs, gz);
	fclose (saida);

	saida = fopen ("camada.txt", "w");
	impressao_camada (saida, M, Y, X, p);
	fclose (saida);

	/*gzz_equivalente(N, M, yp, xp, zp, Y, X, Z_layer, p, gz);

	saida = fopen ("gzz_calculado.txt", "w");
	impressao_componente (saida, N, yp, xp, zp, gz);
	fclose (saida);*/
	
	/* <== impress�o dos arquivos de saida */

	time (&end1);
	
	aux0 = difftime (end1, start1);

	printf("\nPrograma finalizado com sucesso em %.3lf segundos!\n\n", aux0);
	
	fprintf(relatorio, "\nPrograma finalizado com sucesso em %.3lf segundos!\n\n", aux0);
	
	fclose (relatorio);

	system ("PAUSE");

}

double **aloca_matriz_double (FILE *arq, int linha, int coluna) {

    double **m;  /* ponteiro para a matriz */
    int i;

    /* aloca as linhas da matriz */

    m = (double **)calloc(linha, sizeof(double *));

    if (m == NULL) {

        fprintf (arq, "Memoria Insuficiente (linhas)!\n\n");

        fclose (arq);

        system ("PAUSE");
        
        return (NULL);

    }

    /* aloca as colunas da matriz */

    for (i = 0; i < linha; i++ ) {

        m[i] = (double *)calloc(coluna, sizeof(double));

        if (m[i] == NULL) {

            fprintf (arq, "Memoria Insuficiente (colunas)!\n\n");

            fclose (arq);

            system ("PAUSE");
            
            return (NULL);

        }

    }

    return (m); /* retorna o ponteiro para a matriz */

}


double **libera_matriz_double (int linha, double **m) {
      
    int  i;  /* variavel auxiliar */
    
    if (m == NULL) { 
          
        return (NULL);
        
    }
    
    for (i = 0; i < linha; i++) { 
        
        free (m[i]); /* libera as linhas da matriz */
        
    }
    
    free (m); /* libera a matriz */
        
    return (NULL); /* retorna um ponteiro nulo */
    
}

double *aloca_vetor_double (FILE *arq, int tamanho) {
       
    double *v; /* ponteiro para o vetor */
    
    v = (double *)calloc(tamanho, sizeof(double));
        
    if (v == NULL) { /*** verifica se h� mem�ria suficiente ***/
          
        fprintf (arq, "Memoria Insuficiente!\n\n");
        
        fclose (arq);
        
        system ("PAUSE");
        
		return (NULL);
        
    }
     
    return (v); /* retorna o ponteiro para o vetor */
       			           
}

double *libera_vetor_double (double *v) {
      
    if (v == NULL) {
          
        return (NULL);
        
    }
    
    free(v); /* libera o vetor */
    
    return (NULL); /* retorna o ponteiro */
    
}

double sist_lin_LU (FILE *relatorio, int M, double **GtG, double *Gtd, double *p, double uridge) {

	int i, j, k, l;
	double *b, **L;
	double aux0, aux1;

	/************* decomposi��o LU da matriz (GtG + uridgeI) ==> ******************/
	printf ("   Decomposicao LU da matriz GtG\n");

	L = aloca_matriz_double (relatorio, M, M);

	/**** primeira linha de U, primeira coluna de L e diagonal principal de L ****/

	aux0 = 0.01*GtG[0][0];
	aux1 = uridge*aux0;

	L[0][0] = GtG[0][0] + aux1;

	for (i = 1; i < M; i++) {

		L[0][i] = GtG[0][i];
		L[i][0] = (double)(GtG[i][0]/L[0][0]);

	}

	/**** varre as linhas de U e as colunas de L ****/
	for (i = 1; i < M; i++) {

		/**** i-�sima linha de U ****/
		j = i;

			for (k = 0; k < i; k++) {

				L[i][j] -= L[i][k]*L[k][j];

			}

			aux0 = 0.01*GtG[i][j];
			aux1 = uridge*aux0;

			L[i][j] += GtG[i][j] + aux1;

		for (j = i+1; j < M; j++) {

			for (k = 0; k < i; k++) {

				L[i][j] -= L[i][k]*L[k][j];

			}

			L[i][j] += GtG[i][j];

		}

		/**** i-�sima coluna de L ****/
		for (j = i+1; j < M; j++) {

			for (k = 0; k < i; k++) {

				L[j][i] -= L[j][k]*L[k][i];

			}

			L[j][i] += GtG[j][i];

			L[j][i] = (double)(L[j][i]/L[i][i]);

		}

	}

	/*********** <== decomposi��o LU da matriz (GtG + uridgeI) **************/

	/******* resolu��o do sistema linear (GtG uridgeI)p = Gtd ==> ***********/

	b = aloca_vetor_double(relatorio, M);

	/********** (GtG uridgeI)p = Gtd, LUp = Gtd, Lb = Gtd, Up = b ***********/

	/**** resolu��o do sistema Lb = Gtd ***/
	printf ("   Resolucao do sistema Lb = Gtd\n");
	
	for (k = 0; k < M; k++) {

		b[k] = 0.0;

		for (l = 0; l < k; l++) {

			b[k] -= L[k][l]*b[l];

		}

		b[k] += Gtd[k];

	}

	/*** resolu��o do sistema Up = b ***/
	printf ("   Resolucao do sistema Up = b\n\n");
	
	for (j = M-1; j >= 0; j--) {

		p[j] = 0.0;

		for (k = j+1; k < M; k++) {

			p[j] -= L[j][k]*p[k];

		}

		p[j] += b[j];

		p[j] = (double)(p[j]/L[j][j]);

	}

	/******* <== resolu��o do sistema linear (GtG uridgeI)p = Gtd ***********/

	b = libera_vetor_double(b);
	L = libera_matriz_double (M, L);

	return 0 ;

}

/* Fun��o gasdev do cap�tulo sete (Random Numbers) do Numerical Recipes */

/* Returns a normally distributed deviate with zero mean and unit variance, using ran1(idum)
as the source of uniform deviates. */

float gasdev(long *idum) {

	float ran1(long *idum);
	static int iset=0;
	static float gset;
	float fac,rsq,v1,v2;

	/* We don't have an extra deviate handy, so pick two uniform numbers in the
	square extending from -1 to +1 in each direction, see if they are in the unit
	circle, and if they are not, try again. */

	if (iset == 0) {

		do {

			v1=2.0*ran1(idum)-1.0;

			v2=2.0*ran1(idum)-1.0;

			rsq=v1*v1+v2*v2;

		} while (rsq >= 1.0 || rsq == 0.0);

		fac=sqrt(-2.0*log(rsq)/rsq);

		/* Now make the Box-Muller transformation to get two normal deviates. Return one and
		save the other for next time. */

		gset=v1*fac;

		iset=1; /* Set flag. */

		return v2*fac;

	}

	/* We have an extra deviate handy, so unset the flag, and return it. */

	else {

		iset=0;

		return gset;

	}

}

/* Fun��o ran1 do cap�tulo sete (Random Numbers) do Numerical Recipes */

/* "Minimal" random number generator of Park and Miller with Bays-Durham shuffle and added
safeguards. Returns a uniform random deviate between 0.0 and 1.0 (exclusive of the endpoint
values). Call with idum a negative integer to initialize; thereafter, do not alter idum between
successive deviates in a sequence. RNMX should approximate the largest floating value that is
less than 1. */

float ran1(long *idum) {

	int j;
	long k;
	static long iy=0;
	static long iv[NTAB];
	float temp;

	if (*idum <= 0 || !iy) { /* Initialize. */

		if (-(*idum) < 1) *idum=1; /* Be sure to prevent idum = 0. */

		else *idum = -(*idum);

		for (j=NTAB+7;j>=0;j--) { /* Load the shuffle table (after 8 warm-ups). */

			k=(*idum)/IQ;

			*idum=IA*(*idum-k*IQ)-IR*k;

			if (*idum < 0) *idum += IM;

			if (j < NTAB) iv[j] = *idum;

		}

		iy=iv[0];

	}

	/* Start here when not initializing. */
	k=(*idum)/IQ;

	/* Compute idum=(IA*idum) % IM without overflows by Schrage's method. */
	*idum=IA*(*idum-k*IQ)-IR*k;

	if (*idum < 0) *idum += IM;

	/* Will be in the range 0..NTAB-1. */
	j=iy/NDIV;

	/* Output previously stored value and refill the shuffe table. */
	iy=iv[j];

	iv[j] = *idum;

	/* Because users don't expect endpoint values. */
	if ((temp=AM*iy) > RNMX) return RNMX;

	else return temp;

}

void leitura_parametros (FILE *relatorio, int *N, double *uridge, double *usuavidade, double *Z_layer, int *grau, double *stdev_dados, int *semente, int *Npolx, int *Npoly, int *Nspolx, int *Nspoly, double *lambidax, double *lambiday) {

	char str[20];
	
	FILE *entrada;

	sprintf (str, "parametros.txt");

	if (fopen(str, "r") == NULL) {

		fprintf (relatorio, "Arquivo %s nao encontrado!\n\n", str);

		fclose (relatorio);

		printf ("Erro!\n\n");

		system ("PAUSE");

		return 0;

	}

	entrada = fopen(str, "r");

	fscanf (entrada, "%d %lf %lf %lf %d", N, uridge, usuavidade, Z_layer, grau);

	fscanf(entrada, "%lf %d", stdev_dados, semente);
	
	fscanf(entrada, "%d %d %d %d", Npolx, Npoly, Nspolx, Nspoly);
	
	fscanf(entrada, "%lf %lf", lambidax, lambiday);
	
	idum = semente;

	fclose (entrada);

}

void leitura_dados(FILE *relatorio, int N, double *Xp, double *Yp, double *Zp, double *gobs, double *xmin, double *xmax, double *ymin, double *ymax, double stdev_dados) {

	int i;
	double aux;
	char str[20];
	
	FILE *entrada;

	sprintf (str, "gzobs.txt");

	if (fopen(str, "r") == NULL) {

		fprintf (relatorio, "Arquivo %s nao encontrado!\n\n", str);

		fclose (relatorio);

		printf ("Erro!\n\n");

		system ("PAUSE");

		return 0;

	}

	entrada = fopen(str, "r");

	if (fscanf(entrada, "%lf %lf %lf %lf", &Yp[0], &Xp[0], &Zp[0], &gobs[0]) != 4) {

		fprintf(relatorio, "Erro na leitura do arquivo %s!\n\n", str);

		fclose (relatorio);

		printf ("Erro!\n\n");

		system ("PAUSE");

		return 0;

	}

	gobs[0] += stdev_dados*gasdev(&idum);

	(*xmin) = Xp[0];
	(*xmax) = Xp[0];
	(*ymin) = Yp[0];
	(*ymax) = Yp[0];

	for (i = 1; i < N; i++) {

		if (fscanf(entrada, "%lf %lf %lf %lf", &Yp[i], &Xp[i], &Zp[i], &gobs[i]) != 4) {

			fprintf(relatorio, "Erro na leitura do arquivo %s!\n\n", str);

			fclose (relatorio);

			printf ("Erro!\n\n");

			system ("PAUSE");

			return 0;

		}

		gobs[i] += stdev_dados*gasdev(&idum);

		/* Determina��o dos limites m�ximo e m�nimo em X e Y */
		if ((*xmin) > Xp[i]) {

			(*xmin) = Xp[i];

		}
		if ((*xmax) < Xp[i]) {

			(*xmax) = Xp[i];

		}
		if ((*ymin) > Yp[i]) {

			(*ymin) = Yp[i];

		}
		if ((*ymax) < Yp[i]) {

			(*ymax) = Yp[i];

		}

	}
	
	fclose (entrada);

}

void linha_a (int i, int M, double *Y, double *X, double Z_layer, double *yp, double *xp, double *zp, double *a) { 

	int j;
	double aux1, aux2, aux3, aux4, aux5;

	for (j = 0; j < M; j++) {

		a[j] = gz_monopolo(i, yp, xp, zp, j, Y, X, Z_layer);
	
	}

}

void calcula_BtAtAB (int N, int Q, int Npol, int Nspol, int grau, int M, int H, double *Y, double *X, double Z_layer, double *yp, double *xp, double *zp, double *a, double **B, double *g, double **GtG, double *Gtgzobs, double *gzobs, double *fator_normalizacao) {

	double aux0;
	int i, j, k, k1, k2, l, m, n;

	///* percorre as N linhas da matriz A */
	//for (i = 0; i < N; i++) { /* for i */

	//	/* C�lculo da i-�sima linha da matriz A */
	//	linha_a (i, M, X, Y, Z_layer, xp, yp, zp, a);

	//	for (j = 0; j < H; j++) { /* for j */

	//		/* Produto escalar entre a i-�sima linha de A
	//		e a j-�sima coluna de B ==> */
	//		g[j] = a[0]*B[0][j];

	//		for (k = 1; k < M; k++) {
	//				
	//			g[j] += a[k]*B[k][j];
	//				
	//		}					
	//		/* <== Produto escalar entre a i-�sima linha de A
	//		e a j-�sima coluna de B */

	//		/* C�mputo de uma parcela do j-�simo
	//		elemento do vetor Gtgzobs */
	//		Gtgzobs[j] += g[j]*gzobs[i];

	//		for (k = 0; k <= j; k++) {

	//			/* C�mputo de uma parcela do
	//			elemento jk da matriz GtG */
	//			GtG[j][k] += g[j]*g[k];

	//		}
	//				
	//		printf ("i = %5d N = %5d j = %5d H = %5d\n", i, N, j, H);
	//			
	//	} /* for j */

	//} /* for i */

	/* percorre as N linhas da matriz A */
	for (i = 0; i < N; i++) { /* for i */

		/* C�lculo da i-�sima linha da matriz A */
		linha_a (i, M, Y, X, Z_layer, yp, xp, zp, a);

		for (l = 0, j = 0; j < Npol; j++) { /* for j */
		
			k1 = (j*Nspol);
			k2 = k1 + Nspol;

			for (k = 0; k < Q; k++, l++) { /* for k */
			
				/* Produto escalar entre a i-�sima linha de A
				e a l-�sima coluna de B ==> */
				g[l] = a[k1]*B[k1][l];

				for (m = (k1+1); m < k2; m++) {
						
					g[l] += a[m]*B[m][l];
						
				}					
				/* <== Produto escalar entre a i-�sima linha de A
				e a l-�sima coluna de B */

				/* C�mputo de uma parcela do l-�simo
				elemento do vetor Gtgzobs */
				Gtgzobs[l] += g[l]*gzobs[i];

				for (n = 0; n <= l; n++) {

					/* C�mputo de uma parcela do
					elemento nl da matriz GtG */
					/* GtG[l][n] += g[l]*g[n]; preenche a triangular inferior de GtG */
					GtG[n][l] += g[l]*g[n]; /* preenche a triangular superior de GtG */

				}
						
				printf ("i = %5d N = %5d l = %5d H = %5d\n", i, N, l, H);
				
			} /* for k */

		} /* for j */

	} /* for i */
	
	for (aux0 = 0.0, i = 0; i < H; i++) {

		aux0 += GtG[i][i];

	}

	aux0 = (double)(aux0/H);
	//aux0 = (double)(1.0/aux0);
	
	(*fator_normalizacao) = aux0;

	//for (i = 0; i < H; i++) {

	//	for (j = 0; j <= i; j++) {

	//		GtG[j][i] *= aux0;

	//	}

	//}

}

void calcula_BtRtRB (int H, int Q, int Npolx, int Npoly, int Nspol, int Nsy, int Nspolx, int Nspoly, double **B, double *g, double **GtG, double fator_normalizacao, double usuavidade) {

	int i, j, k, l, m, n, o, p, q;
	int aux1, aux2;
	double aux0;

	aux1 = Nsy*Nspolx;
	aux2 = ((Nspolx - 1)*Nspoly) + 1;
	
	/* Suavidade na dire��o y ==> */
	for (i = 0; i < Npolx; i++) { /* for i */

		/*l = Nspoly - 1 + (i*Nsy*Nspolx);*/
		l = Nspoly - 1 + (i*aux1);

		for (j = 0; j < (Npoly - 1); j++) { /* for j */

			/* a impress�o abaixo � para avaliar a velocidade do programa, de maneira visual */
			printf ("i = %5d Npolx = %5d j = %5d (Npoly - 1) = %5d\n", (i+1), Npolx, (j+1), (Npoly - 1));

			for (k = 0; k < Nspolx; k++, l += Nspoly) { /* for k */

				/*for (m = 0; m < H; m++) { * for m * */
				o = (l/Nspol);
				o *= Q;
				p = o + (2*Q);
				
				for (m = o; m < p; m++) {

					/* Produto escalar entre a l-�sima linha de R e a m-�sima coluna de B */
					/*g[m] = B[l][m] - B[(l + ((Nspolx - 1)*Nspoly) + 1)][m];*/
					g[m] = B[l][m] - B[(l + aux2)][m];

					/*for (n = 0; n <= m; n++) { * for n * */
					for (n = o; n <= m; n++) {

						/* C�mputo de uma parcela do
						elemento nm da matriz GtG */
						/*GtG[m][n] += g[m]*g[n]; preenche a triangular inferior de GtG */
						GtG[n][m] += g[m]*g[n]; /* preenche a triangular superior de GtG */

					} /* for n */

				} /* for m */
				
			} /* for k */

		} /* for j */

	} /* for i */
	/* <== Suavidade na dire��o y */
	
	aux1 = Nspoly*(Nspolx - 1);
	aux2 = ((Npoly - 1)*Nspol) + Nspoly;
	
	q = (aux2/Q);
	q *= Q;
	q ++;

	/* Suavidade na dire��o x ==> */
	for (l = 0, i = 0; i < Npoly; i++) { /* for i */

		for (j = 0; j < (Npolx - 1); j++) { /* for j */

		/* a impress�o abaixo � para avaliar a velocidade do programa, de maneira visual */
		printf ("i = %5d Npoly = %5d j = %5d (Npolx - 1) = %5d\n", (i+1), Npoly, (j+1), (Npolx - 1));

			/*l += Nspoly*(Nspolx - 1);*/
			l += aux1;

			for (k = 0; k < Nspoly; k++, l++) { /* for k */

				/*for (m = 0; m < H; m++) { * for m * */

				o = (l/Nspol);
				o *= Q;
				p = o + q;
				
				if (p > H) {
				
					p = H;
					
				}
				
				for (m = o; m < p; m++) {

					/* Produto escalar entre a l-�sima linha de R e a m-�sima coluna de B */
					/*g[m] = B[l][m] - B[(l + ((Npoly - 1)*Nspol) + Nspoly)][m];*/
					g[m] = B[l][m] - B[(l + aux2)][m];

					/*for (n = 0; n <= m; n++) { * for n * */
					for (n = o; n <= m; n++) {

						/* C�mputo de uma parcela do
						elemento nm da matriz GtG */
						/* GtG[m][n] += g[m]*g[n]; preenche a triangular inferior de GtG */
						GtG[n][m] += g[m]*g[n]; /* preenche a triangular superior de GtG */

					} /* for n */

				} /* for m */

			} /* for k */

		} /* for j */

	} /* for i */
	/* <== Suavidade na dire��o x */

	for (aux0 = 0.0, i = 0; i < H; i++) {

		aux0 += GtG[i][i];

	}

	aux0 = (double)(aux0/H);
	aux0 = (double)(fator_normalizacao/aux0);

	usuavidade *= aux0;

	/* Multiplica a parte de GtG acima da diagonal principal
	pelo par�metro de regulariza��o usuavidade */
	for (i = 0; i < H; i++) {

		for (j = 0; j <= i; j++) {

			GtG[j][i] *= usuavidade;

		}

	}

}

void gz_equivalente(int N, int M, double *yp, double *xp, double *zp, double *Y, double *X, double Z_layer, double *p, double *gz) {

	int i, j;

	for (i = 0; i < N; i++) {
	
		gz[i] = 0.0;
	
		for (j = 0; j < M; j++) {
		
			gz[i] += gz_monopolo(i, yp, xp, zp, j, Y, X, Z_layer)*p[j];
		
		}
	
	}

}

double gz_monopolo(int i, double *yp, double *xp, double *zp, int j, double *Y, double *X, double Z_layer) { 

	double aux0, aux1, aux2, aux3, aux4, aux5, aux6;

	aux1 = (xp[i] - X[j]); /* x */
	aux2 = (yp[i] - Y[j]); /* y */
	aux3 = (zp[i] - Z_layer); /* z */
				
	aux4 = pow(aux1, 2) + pow(aux2, 2) + pow(aux3, 2); /* r^2 */
	aux5 = pow(aux4, 1.5); /* r^3 */

	/* -z/r^3 */			
	aux0 = (double)((-aux3)/aux5);

	return aux0*GRAV_CONSTANT*MSTOMGAL;

}

void coordenadas_fontes(int Npolx, int Npoly, int Nspolx, int Nspoly, double dx, double dy, double X1, double Y1, double *X, double *Y) {

	int i, j, k, l, m;
	double x, y, x0, y0;

	for (m = 0, i = 0; i < Npolx; i++) {
	
		x0 = X1 + (i*Nspolx*dx);
		
		for (j = 0; j < Npoly; j++) {
		
			y0 = Y1 + (j*Nspoly*dy);
			
			for (k = 0, x = x0; k < Nspolx; k++, x += dx) {
			
				for (l = 0, y = y0; l < Nspoly; l++, y += dy, m++) {
			
					Y[m] = y;
					X[m] = x;
			
				}
			
			}
		
		}
	
	}

}

void impressao_componente(FILE *arquivo, int N, double *y, double *x, double *z, double *gzobs, double *gzcalc) {

	int i;

	fprintf (arquivo , "              y              x              z           gzobs          gzcalc       diferenca\n");

	for (i = 0; i < N; i++) {

		fprintf (arquivo, "%15.3lf %15.3lf %15.3lf %15.3lf %15.3lf %15.3E\n", y[i], x[i], z[i], gzobs[i], gzcalc[i], (gzcalc[i] - gzobs[i]));

	}

}

void calcula_densidades(int M, int H, double *c, double *p, double **B) {

	int i, j;

	for (i = 0; i < M; i++) {
	
		p[i] = 0.0;
		
		for (j = 0; j < H; j++) {
		
			p[i] += B[i][j]*c[j];
		
		}
	
	
	}
	
	/*for (i = 0; i < H; i++) {

		for (j = 0; j < M; j++) {

			p[j] += c[i]*B[j][i];
					
		}
					
	} */

}

void impressao_camada(FILE *arquivo, int M, double *Y, double *X, double *p) {

	int i;

	fprintf (arquivo , "              Y               X               p\n");

	for (i = 0; i < M; i++) {

		fprintf (arquivo, "%15.3lf %15.3lf %15.5E\n", Y[i], X[i], p[i]);

	}

}

void coluna_b (int M, int j, int Nspol, int l, int k, double *X, double *Y, double *b) {

	int i, i_inicial, i_final;

	i_inicial = (j*Nspol);
	i_final = i_inicial + Nspol;

	for (i = 0; i < M; i++) {

		b[i] = 0.0;

	}

	for (i = i_inicial; i < i_final; i++) {	

		b[i] = pow(Y[i], l)*pow(X[i], (k-l));
	
	}
	
}


void calcula_B (int Npol, int Nspol, int grau, double *Y, double *X, double **B, double Y1, double Y2, double X1, double X2) {

	int i, i_inicial, i_final;
	int j, k, l, m;
	double Y_medio, X_medio, constante_ponderacao;
	
	Y_medio = (Y2 - Y1)*0.5;
	X_medio = (X2 - X1)*0.5;

	constante_ponderacao = (Y_medio + X_medio)*0.5;
	constante_ponderacao = (double)(1.0/constante_ponderacao);

	/* os pr�ximos 3 la�os percorrem as H colunas da matriz B */
	for (m = 0, j = 0; j < Npol; j++) { /* for j */
			
		i_inicial = (j*Nspol);
		i_final = i_inicial + Nspol;

		for (k = 0; k <= grau; k++) { /* for k */
			
			for (l = 0; l <= k; l++, m++) { /* for l */

				/* C�lculo da m-�sima coluna da matriz B ==> */
				for (i = i_inicial; i < i_final; i++) {

					/*B[i][m] = constante_ponderacao*pow(Y[i], l)*pow(X[i], (k-l));*/
					B[i][m] = pow(Y[i], l)*pow(X[i], (k-l));
					
				}
				/* <== C�lculo da m-�sima coluna da matriz B */

			} /* for l */

		} /* for k */

	} /* for j */

}

void choldc(FILE *relatorio, double **a, int n, double *p) {
/*Given a positive-definite symmetric matrix a[1..n][1..n], this routine constructs its Cholesky
decomposition, A = LLt . On input, only the upper triangle of a need be given; it is not
modified. The Cholesky factor L is returned in the lower triangle of a, except for its diagonal
elements which are returned in p[1..n].*/

	int i,j,k;
	double sum;
	
	for (i = 0; i < n; i++) {
	
		j = i;
		
			for (sum = a[i][j], k = i-1; k >= 0; k--) {
			
				sum -= a[i][k]*a[j][k];
				
			}

			if (sum <= 0.0) { /* a, with rounding errors, is not positive definite */ 

				fprintf (relatorio, "\nCholesky falhou!!\n\n");
				
				fclose (relatorio);
				
				abort();
					
			}
			
			p[i] = sqrt(sum);

		for (j = (i+1); j < n; j++) {
		
			for (sum = a[i][j], k = i-1; k >= 0; k--) {
			
				sum -= a[i][k]*a[j][k];
				
			}

			a[j][i] = (double)(sum/p[i]);
		
		}

	}
	
}

void incorpora_ridge (int H, double **GtG, double uridge, double fator_normalizacao) {

	int i;
	double aux0, aux1;
	
	if (uridge != 0.0) {
	
		aux1 = uridge*fator_normalizacao;

		for (i = 0; i < H; i++) {

			GtG[i][i] += aux1;
	
		}

	}

}

void cholsl(double **a, int n, double *p, double *b, double *x) {

/*Solves the set of n linear equations Ax = b, where a is a positive-definite symmetric matrix.
a[1..n][1..n] and p[1..n] are input as the output of the routine choldc. Only the lower
triangle of a is accessed. b[1..n] is input as the right-hand side vector. The solution vector is
returned in x[1..n]. a, n, and p are not modified and can be left in place for successive calls
with diferent right-hand sides b. b is not modified unless you identify b and x in the calling
sequence, which is allowed. */

	int i,k;
	double sum;

	for (i = 0; i < n; i++) { /* Solve Ly = b, storing y in x. */

		for (sum = b[i], k = i-1; k >= 0; k--) {
		
			sum -= a[i][k]*x[k];
			
		}
		
		x[i] = (double)(sum/p[i]);

	}

	for (i = (n - 1); i >= 0; i--) { /* Solve Ltx = y. */

		for (sum = x[i], k = i+1; k < n; k++) {
		
			sum -= a[k][i]*x[k];
			
		}
		
		x[i] = (double)(sum/p[i]);

	}
	
}

void gzz_equivalente(int N, int M, double *yp, double *xp, double *zp, double *Y, double *X, double Z_layer, double *p, double *gzz) {

	int i, j;

	for (i = 0; i < N; i++) {
	
		gzz[i] = 0.0;
	
		for (j = 0; j < M; j++) {
		
			gzz[i] += gzz_monopolo(i, yp, xp, zp, j, Y, X, Z_layer)*p[j];
		
		}
	
	}

}

double gzz_monopolo(int i, double *yp, double *xp, double *zp, int j, double *Y, double *X, double Z_layer) { 

	double aux0, aux1, aux2, aux3, aux4, aux5, aux6;

	aux1 = pow((xp[i] - X[j]), 2); /* x^2 */
	aux2 = pow((yp[i] - Y[j]), 2); /* y^2 */
	aux3 = pow((zp[i] - Z_layer), 2); /* z^2 */
			
	aux4 = aux1 + aux2 + aux3; /* r^2 */
	aux5 = pow(aux4, 1.5);     /* r^3 */
	aux6 = pow(aux4, 2.5);     /* r^5 */

	/* (3*z^2)/r^5 - (1/r^3)  */ 			
	aux0 = ((double)((3*aux3)/aux6)) - ((double)(1.0/aux5));

	return aux0*GRAV_CONSTANT*1E9;

}