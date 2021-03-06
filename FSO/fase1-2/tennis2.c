/*****************************************************************************/
/*									     */
/*				     Tennis0.c				     */
/*									     */
/*  Programa inicial d'exemple per a les practiques 2 i 3 de ISO.	     */
/*     Es tracta del joc del tennis: es dibuixa un camp de joc rectangular    */
/*     amb una porteria a cada costat, una paleta per l'usuari, una paleta   */
/*     per l'ordinador i una pilota que va rebotant per tot arreu; l'usuari  */
/*     disposa de dues tecles per controlar la seva paleta, mentre que l'or- */
/*     dinador mou la seva automaticament (amunt i avall). Evidentment, es   */
/*     tracta d'intentar col.locar la pilota a la porteria de l'ordinador    */
/*     (porteria de la dreta), abans que l'ordinador aconseguixi col.locar   */
/*     la pilota dins la porteria de l'usuari (porteria de l'esquerra).      */
/*									     */
/*  Arguments del programa:						     */
/*     per controlar la posicio de tots els elements del joc, cal indicar    */
/*     el nom d'un fitxer de text que contindra la seguent informacio:	     */
/*		n_fil n_col m_por l_pal					     */
/*		pil_pf pil_pc pil_vf pil_vc				     */
/*		ipo_pf ipo_pc po_vf					     */
/*									     */
/*     on 'n_fil', 'n_col' son les dimensions del taulell de joc, 'm_por'    */
/*     es la mida de les dues porteries, 'l_pal' es la longitud de les dues  */
/*     paletes; 'pil_pf', 'pil_pc' es la posicio inicial (fila,columna) de   */
/*     la pilota, mentre que 'pil_vf', 'pil_vc' es la velocitat inicial;     */
/*     finalment, 'ipo_pf', 'ipo_pc' indicara la posicio del primer caracter */
/*     de la paleta de l'ordinador, mentre que la seva velocitat vertical    */
/*     ve determinada pel parametre 'po_fv'.				     */
/*									     */
/*     A mes, es podra afegir un segon argument opcional per indicar el      */
/*     retard de moviment de la pilota i la paleta de l'ordinador (en ms);   */
/*     el valor d'aquest parametre per defecte es 100 (1 decima de segon).   */
/*									     */
/*  Compilar i executar:					  	     */
/*     El programa invoca les funcions definides en 'winsuport.o', les       */
/*     quals proporcionen una interficie senzilla per a crear una finestra   */
/*     de text on es poden imprimir caracters en posicions especifiques de   */
/*     la pantalla (basada en CURSES); per tant, el programa necessita ser   */
/*     compilat amb la llibreria 'curses':				     */
/*									     */
/*	   $ gcc tennis0.c winsuport.o -o tennis0 -lcurses		     */
/*	   $ tennis0 fit_param [retard]					     */
/*									     */
/*  Codis de retorn:						  	     */
/*     El programa retorna algun dels seguents codis al SO:		     */
/*	0  ==>  funcionament normal					     */
/*	1  ==>  numero d'arguments incorrecte 				     */
/*	2  ==>  fitxer no accessible					     */
/*	3  ==>  dimensions del taulell incorrectes			     */
/*	4  ==>  parametres de la pilota incorrectes			     */
/*	5  ==>  parametres d'alguna de les paletes incorrectes		     */
/*	6  ==>  no s'ha pogut crear el camp de joc (no pot iniciar CURSES)   */
/*****************************************************************************/



#include <stdio.h>		/* incloure definicions de funcions estandard */
#include <stdlib.h>
#include "winsuport.h"		/* incloure definicions de funcions propies */
#include <pthread.h>



#define MIN_FIL 7		/* definir limits de variables globals */
#define MAX_FIL 25
#define MIN_COL 10
#define MAX_COL 80
#define MIN_PAL 3
#define MIN_VEL -1.0
#define MAX_VEL 1.0

int num_oponents;
pthread_t tid[10];
/* variables globals */
int n_fil, n_col, m_por;	/* dimensions del taulell i porteries */
int l_pal;			/* longitud de les paletes */
float v_pal[9];			/* velocitat de la paleta del programa */

int ipu_pf, ipu_pc;      	/* posicio del la paleta d'usuari */
int ipo_pf[9], ipo_pc[9];      	/* posicio del la paleta de l'ordinador */
float po_pf[9];	/* pos. vertical de la paleta de l'ordinador, en valor real */
int f_h[9];

int ipil_pf, ipil_pc;		/* posicio de la pilota, en valor enter */
float pil_pf, pil_pc;		/* posicio de la pilota, en valor real */
float pil_vf, pil_vc;		/* velocitat de la pilota, en valor real*/

int retard;		/* valor del retard de moviment, en mil.lisegons */
int cont;
int tecla;
pthread_mutex_t mutex1= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3= PTHREAD_MUTEX_INITIALIZER;
FILE *doc;

/* funcio per realitzar la carrega dels parametres de joc emmagatzemats */
/* dins un fitxer de text, el nom del qual es passa per referencia en   */
/* 'nom_fit'; si es detecta algun problema, la funcio avorta l'execucio */
/* enviant un missatge per la sortida d'error i retornant el codi per-	*/
/* tinent al SO (segons comentaris del principi del programa).		*/
void carrega_parametres(const char *nom_fit)
{
  FILE *fit;
  int i;
  fit = fopen(nom_fit,"rt");		/* intenta obrir fitxer */
  if (fit == NULL)
  {	fprintf(stderr,"No s'ha pogut obrir el fitxer \'%s\'\n",nom_fit);
  	exit(2);
  }

  if (!feof(fit)) fscanf(fit,"%d %d %d %d\n",&n_fil,&n_col,&m_por,&l_pal);
  if ((n_fil < MIN_FIL) || (n_fil > MAX_FIL) ||
	(n_col < MIN_COL) || (n_col > MAX_COL) || (m_por < 0) ||
	 (m_por > n_fil-3) || (l_pal < MIN_PAL) || (l_pal > n_fil-3))
  {
	fprintf(stderr,"Error: dimensions del camp de joc incorrectes:\n");
	fprintf(stderr,"\t%d =< n_fil (%d) =< %d\n",MIN_FIL,n_fil,MAX_FIL);
	fprintf(stderr,"\t%d =< n_col (%d) =< %d\n",MIN_COL,n_col,MAX_COL);
	fprintf(stderr,"\t0 =< m_por (%d) =< n_fil-3 (%d)\n",m_por,(n_fil-3));
	fprintf(stderr,"\t%d =< l_pal (%d) =< n_fil-3 (%d)\n",MIN_PAL,l_pal,(n_fil-3));
	fclose(fit);
	exit(3);
  }

  if (!feof(fit)) fscanf(fit,"%d %d %f %f\n",&ipil_pf,&ipil_pc,&pil_vf,&pil_vc);
  if ((ipil_pf < 1) || (ipil_pf > n_fil-3) ||
	(ipil_pc < 1) || (ipil_pc > n_col-2) ||
	(pil_vf < MIN_VEL) || (pil_vf > MAX_VEL) ||
	(pil_vc < MIN_VEL) || (pil_vc > MAX_VEL))
  {
	fprintf(stderr,"Error: parametre pilota incorrectes:\n");
	fprintf(stderr,"\t1 =< ipil_pf (%d) =< n_fil-3 (%d)\n",ipil_pf,(n_fil-3));
	fprintf(stderr,"\t1 =< ipil_pc (%d) =< n_col-2 (%d)\n",ipil_pc,(n_col-2));
	fprintf(stderr,"\t%.1f =< pil_vf (%.1f) =< %.1f\n",MIN_VEL,pil_vf,MAX_VEL);
	fprintf(stderr,"\t%.1f =< pil_vc (%.1f) =< %.1f\n",MIN_VEL,pil_vc,MAX_VEL);
	fclose(fit);
	exit(4);
  }
  i=0;
  do
  { 
  fscanf(fit,"%d %d %f\n",&ipo_pf[i],&ipo_pc[i],&v_pal[i]);
  if ((ipo_pf[i] < 1) || (ipo_pf[i]+l_pal > n_fil-2) ||
	(ipo_pc[i] < 5) || (ipo_pc[i] > n_col-2) ||
	(v_pal[i] < MIN_VEL) || (v_pal[i] > MAX_VEL))
    {
	fprintf(stderr,"Error: parametres paleta ordinador incorrectes:\n");
	fprintf(stderr,"\t1 =< ipo_pf (%d) =< n_fil-l_pal-3 (%d)\n",ipo_pf[i],
			(n_fil-l_pal-3));
	fprintf(stderr,"\t5 =< ipo_pc (%d) =< n_col-2 (%d)\n",ipo_pc[i],
			(n_col-2));
	fprintf(stderr,"\t%.1f =< v_pal (%.1f) =< %.1f\n",MIN_VEL,v_pal[i],MAX_VEL);
	fclose(fit);
	exit(5);
    }
  i++;
  }while(!feof(fit));
  fclose(fit);			/* fitxer carregat: tot OK! */
}




/* funcio per inicialitar les variables i visualitzar l'estat inicial del joc */
int inicialitza_joc(void)
{
  int i,j, i_port, f_port, retwin;
  char strin[51];

  retwin = win_ini(&n_fil,&n_col,'+',INVERS);   /* intenta crear taulell */

  if (retwin < 0)       /* si no pot crear l'entorn de joc amb les curses */
  { fprintf(stderr,"Error en la creacio del taulell de joc:\t");
    switch (retwin)
    {   case -1: fprintf(stderr,"camp de joc ja creat!\n");
                 break;
        case -2: fprintf(stderr,"no s'ha pogut inicialitzar l'entorn de curses!\n");
 		 break;
        case -3: fprintf(stderr,"les mides del camp demanades son massa grans!\n");
                 break;
        case -4: fprintf(stderr,"no s'ha pogut crear la finestra!\n");
                 break;
     }
     return(retwin);
  }

  i_port = n_fil/2 - m_por/2;	    /* crea els forats de la porteria */
  if (n_fil%2 == 0) i_port--;
  if (i_port == 0) i_port=1;
  f_port = i_port + m_por -1;
  for (i = i_port; i <= f_port; i++)
  {	win_escricar(i,0,' ',NO_INV);
	win_escricar(i,n_col-1,' ',NO_INV);
  }


  ipu_pf = n_fil/2; ipu_pc = 3;		/* inicialitzar pos. paletes */
  if (ipu_pf+l_pal >= n_fil-3) ipu_pf = 1;
  for (i=0; i< l_pal; i++)      /* dibuixar paleta inicialment */
  { 
    win_escricar(ipu_pf +i, ipu_pc, '0',INVERS);
  }
  i=0;
  do
  {
  for (j=0; j< l_pal; j++)	    /* dibuixar paleta inicialment */
  {	
	win_escricar(ipo_pf[i] +j, ipo_pc[i], '1',INVERS);
  }
  po_pf[i] = ipo_pf[i];		/* fixar valor real paleta ordinador */
  i++;
  }while(i<num_oponents);
  pil_pf = ipil_pf; pil_pc = ipil_pc;	/* fixar valor real posicio pilota */
  win_escricar(ipil_pf, ipil_pc, '.',INVERS);	/* dibuix inicial pilota */

  sprintf(strin,"Tecles: \'%c\'-> amunt, \'%c\'-> avall, RETURN-> sortir.",
		TEC_AMUNT, TEC_AVALL);
  win_escristr(strin);
  return(0);
}



/* funcio per moure la pilota; retorna un valor amb alguna d'aquestes	*/
/* possibilitats:							*/
/*	-1 ==> la pilota no ha sortit del taulell			*/
/*	 0 ==> la pilota ha sortit per la porteria esquerra		*/
/*	>0 ==> la pilota ha sortit per la porteria dreta		*/
int moure_pilota(void)
{
  
  int f_hp, c_h;
  char rh,rv,rd;
  do
  {
  f_hp = pil_pf + pil_vf;		/* posicio hipotetica de la pilota */
  c_h = pil_pc + pil_vc;
  
  pthread_mutex_lock(&mutex2);
  cont = -1;		/* inicialment suposem que la pilota no surt */
  pthread_mutex_unlock(&mutex2);
  rh = rv = rd = ' ';
  if ((f_hp != ipil_pf) || (c_h != ipil_pc))
  {		/* si posicio hipotetica no coincideix amb la pos. actual */
    if (f_hp != ipil_pf)		/* provar rebot vertical */
    {	
    pthread_mutex_lock(&mutex3);
    rv = win_quincar(f_hp,ipil_pc);	/* veure si hi ha algun obstacle */
	  pthread_mutex_unlock(&mutex3);
  if (rv != ' ')			/* si no hi ha res */
	{   pil_vf = -pil_vf;		/* canvia velocitat vertical */
	    f_hp = pil_pf+pil_vf;	/* actualitza posicio hipotetica */
	}
    }
    if (c_h != ipil_pc)		/* provar rebot horitzontal */
    {	
    pthread_mutex_lock(&mutex3);
    rh = win_quincar(ipil_pf,c_h);	/* veure si hi ha algun obstacle */
    pthread_mutex_unlock(&mutex3);
	if (rh != ' ')			/* si no hi ha res */
	{    pil_vc = -pil_vc;		/* canvia velocitat horitzontal */
	     c_h = pil_pc+pil_vc;	/* actualitza posicio hipotetica */
	}
    }
    if ((f_hp != ipil_pf) && (c_h != ipil_pc))	/* provar rebot diagonal */
    {	pthread_mutex_lock(&mutex3);
      rd = win_quincar(f_hp,c_h);
      pthread_mutex_unlock(&mutex3);
	if (rd != ' ')				/* si no hi ha obstacle */
	{    pil_vf = -pil_vf; pil_vc = -pil_vc;	/* canvia velocitats */
	     f_hp = pil_pf+pil_vf;
	     c_h = pil_pc+pil_vc;		/* actualitza posicio entera */
	}
    }
    pthread_mutex_lock(&mutex3);
    if (win_quincar(f_hp,c_h) == ' ')	/* verificar posicio definitiva */
    {						/* si no hi ha obstacle */
  pthread_mutex_unlock(&mutex3);
  pthread_mutex_lock(&mutex1);  
	win_escricar(ipil_pf,ipil_pc,' ',NO_INV);	/* esborra pilota */
	pthread_mutex_unlock(&mutex1);
  pil_pf += pil_vf; pil_pc += pil_vc;
	ipil_pf = f_hp; ipil_pc = c_h;		/* actualitza posicio actual */
  
	if ((ipil_pc > 0) && (ipil_pc <= n_col)){	/* si no surt */
    pthread_mutex_lock(&mutex1);
		win_escricar(ipil_pf,ipil_pc,'.',INVERS); /* imprimeix pilota */
    pthread_mutex_unlock(&mutex1);
	}else{
    pthread_mutex_lock(&mutex2);
		cont = ipil_pc;	/* codi de finalitzacio de partida */
    pthread_mutex_unlock(&mutex2);}
    }
    pthread_mutex_unlock(&mutex3);
  }
  else { pil_pf += pil_vf; pil_pc += pil_vc; }
  win_retard(retard);
  }while((tecla != TEC_RETURN) && (cont==-1));
  pthread_exit(NULL);
  
}




/* funcio per moure la paleta de l'usuari en funcio de la tecla premuda */
void mou_paleta_usuari(void)
{
  
    do
    {
      tecla = win_gettec();
      pthread_mutex_lock(&mutex3);
      if ((tecla == TEC_AVALL) && (win_quincar(ipu_pf+l_pal,ipu_pc) == ' '))
      {
        pthread_mutex_unlock(&mutex3);
        pthread_mutex_lock(&mutex1);
        win_escricar(ipu_pf,ipu_pc,' ',NO_INV);	   /* esborra primer bloc */
        pthread_mutex_unlock(&mutex1);
        ipu_pf++;					   /* actualitza posicio */
        pthread_mutex_lock(&mutex1);
        win_escricar(ipu_pf+l_pal-1,ipu_pc,'0',INVERS); /* impri. ultim bloc */
        pthread_mutex_unlock(&mutex1);
      }
      pthread_mutex_unlock(&mutex3);
      pthread_mutex_lock(&mutex3);
      if ((tecla == TEC_AMUNT) && (win_quincar(ipu_pf-1,ipu_pc) == ' '))
      {
        pthread_mutex_unlock(&mutex3);
        pthread_mutex_lock(&mutex1);
        win_escricar(ipu_pf+l_pal-1,ipu_pc,' ',NO_INV); /* esborra ultim bloc */
        pthread_mutex_unlock(&mutex1);
        ipu_pf--;					    /* actualitza posicio */
        pthread_mutex_lock(&mutex1);
        win_escricar(ipu_pf,ipu_pc,'0',INVERS);	    /* imprimeix primer bloc */
        pthread_mutex_unlock(&mutex1);
      }
      pthread_mutex_unlock(&mutex3);
      win_retard(retard);
    }while((tecla != TEC_RETURN) && (cont==-1));
    pthread_exit(NULL);
}




/* funcio per moure la paleta de l'ordinador autonomament, en funcio de la */
/* velocitat de la paleta (variable global v_pal) */
void mou_paleta_ordinador(int * i)
{
  
  int index = (int)i;
  
  do
  {
 /* char rh,rv,rd; */
  f_h[index] = po_pf[index] + v_pal[index];		/* posicio hipotetica de la paleta */
  if (f_h[index] != ipo_pf[index])	/* si pos. hipotetica no coincideix amb pos. actual */
  {
    if (v_pal[index] > 0.0)			/* verificar moviment cap avall */
    {
	pthread_mutex_lock(&mutex3);
  if (win_quincar(f_h[index]+l_pal-1,ipo_pc[index]) == ' ')   /* si no hi ha obstacle */
	{
    pthread_mutex_unlock(&mutex3);
    pthread_mutex_lock(&mutex1);
	  win_escricar(ipo_pf[index],ipo_pc[index],' ',NO_INV);      /* esborra primer bloc */
	  pthread_mutex_unlock(&mutex1);
    po_pf[index] += v_pal[index]; ipo_pf[index] = po_pf[index];		/* actualitza posicio */
	  pthread_mutex_lock(&mutex1);
    win_escricar(ipo_pf[index]+l_pal-1,ipo_pc[index],'1',INVERS); /* impr. ultim bloc */
    pthread_mutex_unlock(&mutex1);
	}
	else{
    pthread_mutex_unlock(&mutex3);
    v_pal[index] = -v_pal[index];   /* si hi ha obstacle, canvia el sentit del moviment */
  }		
	   
    }
    else			/* verificar moviment cap amunt */
    {
  pthread_mutex_lock(&mutex3);
	if (win_quincar(f_h[index],ipo_pc[index]) == ' ')        /* si no hi ha obstacle */
	{
    pthread_mutex_unlock(&mutex3);
    pthread_mutex_lock(&mutex1);
	  win_escricar(ipo_pf[index]+l_pal-1,ipo_pc[index],' ',NO_INV); /* esbo. ultim bloc */
	  pthread_mutex_unlock(&mutex1);
    po_pf[index] += v_pal[index]; ipo_pf[index] = po_pf[index];		/* actualitza posicio */
	  pthread_mutex_lock(&mutex1);
    win_escricar(ipo_pf[index],ipo_pc[index],'1',INVERS);	/* impr. primer bloc */
    pthread_mutex_unlock(&mutex1);
	}
	else{		/* si hi ha obstacle, canvia el sentit del moviment */
     pthread_mutex_unlock(&mutex3);
	   v_pal[index] = -v_pal[index];
   }
    }
  }
  else po_pf[index] += v_pal[index]; 	/* actualitza posicio vertical real de la paleta */
  win_retard(retard);
  }while ((tecla != TEC_RETURN) && (cont==-1));
  pthread_exit(NULL);
}




/* programa principal				    */
int main(int n_args, const char *ll_args[])
{
  doc = fopen("log.txt","w");
  pthread_mutex_init(&mutex1, NULL);
  pthread_mutex_init(&mutex2, NULL);
  pthread_mutex_init(&mutex3, NULL);
 /* if ((n_args != 2) && (n_args !=3))
  {	fprintf(stderr,"Comanda: tennis0 fit_param [retard]\n");
  	exit(1);
  }*/
  num_oponents=atoi(ll_args[3]);
  carrega_parametres(ll_args[1]);

  if (n_args == 3) retard = atoi(ll_args[2]);
  else retard = 100;

  
  if (inicialitza_joc() !=0)    /* intenta crear el taulell de joc */
     exit(4);   /* aborta si hi ha algun problema amb taulell */
  
  int i = 1;

  do
  {
    pthread_create(&tid[i+1], NULL, mou_paleta_ordinador,(void *)(i-1));
    i++;
  }while(i<=num_oponents);
 
  pthread_create(&tid[1], NULL, moure_pilota, NULL);
  do
  {
    tecla = win_gettec();
    if(tecla!=0){
      pthread_create(&tid[0], NULL, mou_paleta_usuari, NULL);
    }
  }while(tecla==0);
  
  //do				/********** bucle principal del joc **********/
  //{	tec = win_gettec();
	/*if (tec != 0) mou_paleta_usuari(tec);
	mou_paleta_ordinador();
	cont = moure_pilota();
	win_retard(retard);
  } while ((tec != TEC_RETURN) && (cont==-1));*/
  
  

  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);
  i = 1;
  while (i <= num_oponents)
  {
  pthread_join(tid[i+1], NULL);
  i++;
  }
  win_fi();
  if (tecla == TEC_RETURN) printf("S'ha aturat el joc amb la tecla RETURN!\n");
  else { if (cont == 0) printf("Ha guanyat l'ordinador!%d\n",cont);
         else printf("Ha guanyat l'usuari!\n"); }
  pthread_mutex_destroy(&mutex1);
  pthread_mutex_destroy(&mutex2);
  pthread_mutex_destroy(&mutex3);
  return(0);
}
