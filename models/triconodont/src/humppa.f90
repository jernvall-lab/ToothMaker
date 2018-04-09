!module esclec is to save and load parameter files and save and output tooth morphology files
!module coreop2d is the model itself. It includes the following subroutines
!      subroutine ciinicial          specifies the default initial conditions
!      subroutine dime               allocates the matrices for the initial conditions and sets them     
!      subroutine redime             not in use
!      subroutine posar              specifies the position of cells in the initial conditions
!      subroutine calculmarges       calculates the shape of each epithelial cell (this is the position of its margins)
!      subroutine reaccio_difusio    calculates diffusion of all the molecules between cells
!      subroutine diferenciacio      updates cells differentiation values  
!      subroutine empu               calculates pushing between cells resulting from epithelial growth and border growth
!      subroutine stelate            calculates pushing between cells resulting from buoyancy
!      subroutine pushing            calculates repulsion between neighboring cells
!      subroutine pushingnovei       checks if non-neighbors cells get too close and applies repulsion between them (it never happens for seals)
!      subroutine biaixbl            applies BMP4 concentration in the buccal and lingual borders of the tooth
!      subroutine promig             calculates the nucleus traction by the cell borders
!      subroutine actualitza         updates cell positions
!      subroutine afegircel          calculates where cell divisions occur and adds new cells accordingly
!      subroutine perextrems         identifies which of the cells added in afegircel are in the border of the tooth
!      subroutine iteracio           determines the other by which the subroutines are called (invariable)

!***************************************************************************
!***************  MODUL ****************************************************
!***************************************************************************

module coreop2d   !ATENCIO: versio optimitzada en la que no sumem per ordre

!es com coreop2d pero els marges no poden creixer cap als costats despres de cert valor radibi

!use opengl_gl
implicit none
public :: iteracio,ci,calculmarges,reaccio_difusio,dime,llegirparam,ciinicial

!coreop2d
real*8, public, allocatable  :: malla(:,:)  ! les posicions dels nodes x,y,z
real*8, public, allocatable  :: marge(:,:,:)  ! les posicions dels internodes per cada cel x,y,z
integer, public, allocatable :: vei(:,:)
integer, public, allocatable :: knots(:)
integer, public, allocatable :: nveins(:)
real*8, public, allocatable  :: q2d(:,:)    ! quantitats que son 2d
real*8, public, allocatable  :: q3d(:,:,:)    ! quantitats que son 3d act,inh,fgf,ect,p
real*8, public,allocatable   :: difq3d(:),difq2d(:)
real*8, public, allocatable  :: hmalla(:,:),hvmalla(:,:)
real*8, public, allocatable  :: px(:),py(:),pz(:)

integer, public :: ncels   !numero de celules
integer, public :: ncals   !numero de celules dins del radi real
integer, public :: ncz     !numero de profunditat en z per als calculs de quantitats
integer, parameter, public :: nvmax=30   !numero max de veins
integer, public :: radi
integer, public, parameter :: ng=5,ngg=4
integer, public :: temps,npas
real*8, public,  parameter :: la=1.      !distancia original entre node

!parametres de veritat
real*8, public :: ud,us   !umbral de diferenciacio
real*8, public :: tacre 
real*8, public :: tahor
real*8, public :: acac
real*8, public :: acec
real*8, public :: acaca
real*8, public :: ihac
real*8, public :: ih
real*8, public :: elas
real*8, public :: tadi    ! amplada dels biaixos!taxa de diferenciacio
real*8, public :: crema    !taxa de diferenciacio
real*8, public, parameter :: dmax=2.    !distanica maxima abans de fer un nou node
real*8, public :: bip,bia,bil,bib   !biaixos posterior,anterior,lingual,bucal  pabl
real*8, public :: ampl 
real*8, public :: mu ! degradacio de fgf
real*8, public :: tazmax ! sharpness maxima, indica com el epitli tira cap abaix si no tenim pressio del mesenkima
                         ! no afecta molt amenys que no sigui molt baix, 100 es un bon valor
real*8, public :: radibi ! promitjament
real*8, public :: radibii !radi del centre on apliquem el biaix ap
real*8, public :: fac
real*8, public :: condme ! concentracio min per tenir creixement cap abaix
real*8, public :: tadif ! concentracio min per tenir creixement cap abaix
!semiconstants
real*8, public :: umelas
integer, public :: maxcels

!de implementacio
real*8, parameter ::  delta=0.005D1 , vmin=0.015D1 
integer,public :: nca,icentre,centre,ncils,focus
real*8,public:: x,y,xx,yy
real*8, public :: csu,ssu,csd,ssd,cst,sst,csq,ssq,csc,ssc,css,sss
integer, public :: nnous
integer, public :: nmaa,nmap
integer, public,allocatable :: mmaa(:),mmap(:)

!les tipiques
integer, public :: i,j,k,ii,jj,kk,iii,jjj,kkk,iiii,jjjj,kkkk,iiiii,jjjjj,kkkkk
real*8, public :: a,b,c,d,e,f,g,h,aa,bb,cc,dd,ee,ff,gg,hh,aaa,bbb,ccc,ddd,eee,fff,ggg,hhh,panic

!de visualitzacio
integer, public :: vlinies,vrender,vmarges,vvec,vvecx,vveck,vex,vn
integer, public :: nc
integer, public :: pin,pina ! var a pillar act,inh,fgf,ect,p
integer, public :: submenuid
integer, public :: nivell,kko !nivell al que fem el tall
integer, public :: iti,iteraciototal
character*31, public ::  nfpro

!constants
!real(kind=gldouble), public, parameter ::  pi = 3.141592653589793_gldouble
real*8, public, parameter ::  pii = 31.41592653589793D-1

contains

subroutine ciinicial
  real*8 ua,ub,uc,ux,uy,uz

  !valors de core
  ncz=4
  temps=0
  npas=1
  !de visualitzacio
  vlinies=0
  vrender=1
  vvec=0
  vn=0
  vveck=0
  vvecx=0
  vmarges=0
  vex=0
  pin=1
  pina=1
  nivell=1
  panic=0
  allocate(difq3d(ng))
  allocate(difq2d(ngg))
end subroutine ciinicial

subroutine ci
  real*8 ua,ub,uc,ux,uy,uz

  !valors de core
  temps=0
  npas=1
  !de visualitzacio
  vlinies=1
  vrender=1
  vvec=0
  vn=0
  vveck=0
  vvecx=0
  vmarges=0
  vex=0
  pin=1
  pina=1
  nivell=1
  panic=0
  call dime
end subroutine ci

subroutine dime
  integer, allocatable :: cv(:,:)
  real*8 , allocatable :: cmalla(:,:)
  integer iit

  umelas=1-elas

  !valors de vars
  j=0
  do i=1,radi   ; j=j+i ; end do ; ncals=6*j+1 
  j=0
  do i=1,radi-1 ; j=j+i ; end do ; ncels=6*j+1
  a=pii*0.2D1/0.36D3
  csu=dsin(0*a)  ; ssu=dcos(0*a)
  csd=dsin(60*a)  ; ssd=dcos(60*a)
  cst=dsin(120*a) ; sst=dcos(120*a)
  csq=dsin(180*a) ; ssq=dcos(180*a)
  csc=dsin(240*a) ; ssc=dcos(240*a)
  css=dsin(300*a) ; sss=dcos(300*a)
!  sse=dint(sse*1D14)*1D-14 ; cse=dint(cse*1D14)*1D-14

  !alocatacions
  allocate(cv(ncals,nvmax))
  allocate(cmalla(ncals,3))

  allocate(malla(ncals,3))
  allocate(vei(ncals,nvmax))
  allocate(hmalla(ncals,3))
  allocate(hvmalla(ncals,3))
  allocate(marge(ncals,nvmax,8))
  allocate(knots(ncals))
  allocate(nveins(ncals))
  allocate(q2d(ncals,ngg))
print *,ncals,ncz,ng
  allocate(q3d(ncals,ncz,ng))
  allocate(mmap(radi))
  allocate(mmaa(radi))

  !matrius de visualitzacio
  allocate(px(ncals)) ; allocate(py(ncals)) ; allocate(pz(ncals))

  ampl=radi*0.75

  !valors de zeros
  vei=0. ; nveins=0. ; malla=0. ; q2d=0. ; q3d=0. ; knots=0 ; hmalla=0. ; hvmalla=0.
  
  !valors inicials
  malla(1,1)=0. ; malla(1,2)=0. ; malla(1,3)=1.
  nca=1
  nveins=6

  !valors inicials de quantitats
!  q3d(:ncils,1,1)=1.1 

  !valors inicials de la malla

al: do icentre=1,ncels
    x=malla(icentre,1) ; y=malla(icentre,2)

    !cap a dalt ,1
    xx=x+csu*la ; yy=y+ssu*la ; j=1 ; jj=4 ; call posar
    xx=x+csd*la ; yy=y+ssd*la ; j=2 ; jj=5 ; call posar
    xx=x+cst*la ; yy=y+sst*la ; j=3 ; jj=6 ; call posar
    xx=x+csq*la ; yy=y+ssq*la ; j=4            ; jj=1 ; call posar
    xx=x+csc*la ; yy=y+ssc*la ; j=5 ; jj=2 ; call posar
    xx=x+css*la ; yy=y+sss*la ; j=6 ; jj=3 ; call posar

!    xx=x ; yy=y+la ; j=1 ; jj=4 ; call posar
!    xx=x+csd*la ; yy=y+ssd*la ; j=2 ; jj=5 ; call posar
!    xx=x+csd*la ; yy=y-ssd*la ; j=3 ; jj=6 ; call posar
!    xx=x ; yy=y-la ; j=4            ; jj=1 ; call posar
!    xx=x-csd*la ; yy=y-ssd*la ; j=5 ; jj=2 ; call posar
!    xx=x-csd*la ; yy=y+ssd*la ; j=6 ; jj=3 ; call posar
end do al
do i=2,ncels
  do j=1,nvmax
    if (vei(i,j)>ncels) then ; vei(i,j)=ncals ; end if 
  end do
end do
do k=1,3
do i=2,ncels
  do j=1,nvmax-1
    if (vei(i,j)==ncals.and.vei(i,j+1)==ncals) then 
      do jj=j,nvmax-1
        vei(i,jj)=vei(i,jj+1)
      end do
    end if 
  end do
end do
end do
do i=2,ncels
  k=0
  do j=1,nvmax
    if (vei(i,j)==ncals.and.k==0) then ; k=1 ; cycle ; end if
    if (vei(i,j)==ncals.and.k==1) then ; vei(i,j)=0 ; exit ; end if
  end do
end do
malla=dnint(malla*1D14)*1D-14
do i=1,ncels
  do j=1,3
    if (abs(malla(i,j))<1D-14) malla(i,j)=0. ; end do ; end do

!calcul de distancia original entre nodes

!inversio de forma que els primers son als marges

cv=vei
cmalla=malla
do i=ncels,1,-1
  vei(i,:)=cv(ncels-i+1,:)
  malla(i,:)=cmalla(ncels-i+1,:)
end do

cv=vei
do i=ncels,1,-1
  ii=ncels-i+1
  do jj=1,ncels
    do jjj=1,nvmax
      if (cv(jj,jjj)==i)  vei(jj,jjj)=ii
    end do
  end do
end do

call calculmarges
nveins=3
nveins(1)=6
marge(:,:,4:5)=la
do i=1,ncels
!print *,vei(i,1:6)
end do
centre=ncels
ncils=(radi-1)*6+1
focus=ncils/2-1
if (radi==2) focus=3
!print *,ncils,ncels,ncals,"eeee"
  !valors dels marges  ATENCIO NO ESCALABLE AMB radi, nomes per radi=2
  mmaa=0 ; mmap=0
!  do i=1,radi+1 ; mmap(i)=i ; end do
!  mmap(radi+2)=ncils-3
  do i=1,radi ; mmap(i)=i ; end do
  ii=0
  do i=ncils/2+1,ncils/2+radi ; ii=ii+1 ; mmaa(ii)=i ; end do
!  mmaa(radi+2)=ncils-2
!  mmaa(radi+1)=ncils/2
!  nmaa=radi+2 ; nmap=radi+2
  nmaa=radi ; nmap=radi
!do iit=1,int(radibi*2)
!  do i=1,nmaa
!    malla(mmaa(i),1)=malla(mmaa(i),1)+0.05D1
!  end do
!  do i=1,nmap
!    malla(mmap(i),1)=malla(mmap(i),1)-0.05D1
!  end do
!  malla(4:5,1)=malla(4:5,1)+0.05D1
!  call calculmarges
!  call afegircel
!end do
call calculmarges
!do iit=1,int(bia*2)
!  do i=1,ncils-1
!    if (malla(i,2)>0.) then
!      malla(i,2)=malla(i,2)+0.05D1
!    else
!      if (malla(i,2)<0.) malla(i,2)=malla(i,2)-0.05D1 
!    end if
!  end do
!  call calculmarges
!  call afegircel
!end do
!call calculmarges
!valors inicials de quantitats
q3d=0
end subroutine dime

subroutine redime
  integer, allocatable :: cv(:,:)
  real*8 , allocatable :: cmalla(:,:)
  integer iit

!  ncals=ncels+1

  umelas=1-elas


  !alocatacions
  allocate(cv(ncals,nvmax))
  allocate(cmalla(ncals,3))

  allocate(malla(ncals,3))
  allocate(vei(ncals,nvmax))
  allocate(hmalla(ncals,3))
  allocate(hvmalla(ncals,3))
  allocate(marge(ncals,nvmax,8))
  allocate(knots(ncals))
  allocate(nveins(ncals))
  allocate(q2d(ncals,ngg))
  allocate(q3d(ncals,ncz,ng))
  allocate(mmap(radi))
  allocate(mmaa(radi))

  !matrius de visualitzacio
  allocate(px(ncals)) ; allocate(py(ncals)) ; allocate(pz(ncals))

  ampl=radi*0.75

  !valors de zeros
  vei=0. ; nveins=0. ; malla=0. ; q2d=0. ; q3d=0. ; knots=0 ; hmalla=0. ; hvmalla=0.
  
  !valors inicials
  malla(1,1)=0. ; malla(1,2)=0. ; malla(1,3)=1.
  nca=1
end subroutine redime

subroutine referci
  deallocate (malla) ;  deallocate(q3d)  ; deallocate(px)    ; deallocate(py)     ; deallocate(pz) 
  deallocate (q2d)   ;  deallocate(marge); deallocate(vei)   ; deallocate(nveins) ; deallocate(hmalla)
  deallocate(hvmalla);  deallocate(knots) ; deallocate(mmaa) ; deallocate (mmap)
  call ci 
!  call iteracio(1)
end subroutine

subroutine refercid
  deallocate (malla) ;  deallocate(q3d)  ; deallocate(px) ; deallocate(py)     ; deallocate(pz) 
  deallocate (q2d)   ;  deallocate(marge); deallocate(vei); deallocate(nveins) ; deallocate(hmalla)
  deallocate(hvmalla);  deallocate(knots);  deallocate(mmaa) ; deallocate (mmap)
end subroutine

subroutine posar
al: do i=1,nca 
      if (i==icentre) cycle al
if (dnint(1000000*malla(i,1))==dnint(1000000*xx).and.dnint(1000000*malla(i,2))==dnint(1000000*yy)) then ; 
        do ii=1,nvmax ; if (vei(icentre,ii)==i) then ; return ; end if ;  end do
vei(icentre,j)=i ; vei(i,jj)=icentre ; nveins(i)=nveins(i)+1 ; nveins(icentre)=nveins(icentre)+1 ; return
      end if
    end do al
    nveins(icentre)=nveins(icentre)+1 ; nca=nca+1 ; vei(icentre,j)=nca ; vei(nca,jj)=icentre ; malla(nca,1)=xx ; 
    malla(nca,2)=yy ; malla(nca,3)=1. ; nveins(nca)=nveins(nca)+1
end subroutine posar

subroutine llegirparam
  open(1,file="inex/veure27.dad",status='old',iostat=i)
  !print *,i
45  read (1,*,err=666,end=777) radi
  !print *,radi
  read (1,*,err=666,end=777) tacre,tahor,elas,tadi,crema
  !print *,tacre,"tacre",tahor,"tahor",elas,"elas",crema,"crema"
  read (1,*,err=666,end=777) acac,ihac,acaca,ih,acec
  !print *,acac,"acac",ihac,"ihac",acaca,"acaca",ih,"ih",acec,"acec"
  read (1,*,err=666,end=777) difq3d
  !print *,difq3d,"difusions"
  read (1,*,err=666,end=777) difq2d
  !print *,difq2d,"difusions 2D"
  read (1,*,err=666,end=777) bip,bia,bib,bil
  !print *,bip,bia,bib,bil
  goto 45
666  tacre=29D-1  ; tahor=0.1D1 ; acac=0.1D1 ; acaca=1D-3   ; ihac=10D1 ; ih=1D-2
     difq3d=0.1D1 ; elas=0.8  ; difq2d=1. ; difq3d=1.
     bip=0.       ; bia=0.      ; bib=0.     ; bil=0.
     print *,"error de lectura dels fitxer de dades, dades no pillades"
777 close(1)
end subroutine llegirparam

subroutine calculmarges
  real*8 cont
  integer kl

  marge(:,:,1:3)=0.
  do i=1,ncels
    aa=0. ; bb=0. ; cc=0. ; kl=0
    do j=1,nvmax
      if (vei(i,j)/=0.) then
        a=0. ; b=0. ; c=0. ; cont=0
        iii=i 
        a=malla(i,1) ; b=malla(i,2) ; c=malla(i,3) ; cont=1
        ii=vei(i,j)  
        if (ii>ncels) then
          do jj=j-1,1,-1
            if (vei(i,jj)/=0) then  
              if (vei(i,jj)<ncels+1) then ; ii=vei(i,jj) ; a=a+malla(ii,1) ; b=b+malla(ii,2) ; c=c+malla(ii,3) 
              cont=cont+1 ; goto 77 ; else ; goto 77 ; end if
            end if 
          end do 
          do jj=nvmax,j+1,-1
            if (vei(i,jj)/=0) then  
              if (vei(i,jj)<ncels+1) then ; ii=vei(i,jj) ; a=a+malla(ii,1) ; b=b+malla(ii,2) ; c=c+malla(ii,3) 
              cont=cont+1 ; goto 77 ; else ; goto 77 ; end if
            end if 
          end do 
          goto 77 
        end if
66      if (ii==i) goto 77
        kl=kl+1
        if (kl>100) then
          do jj=j-1,1,-1
            if (vei(i,jj)/=0) then  
              if (vei(i,jj)<ncels+1) then ; ii=vei(i,jj) ; a=a+malla(ii,1) ; b=b+malla(ii,2) ; c=c+malla(ii,3) 
                cont=cont+1 ; goto 77 ; else ; goto 77 ; end if
              end if 
           end do 
           do jj=nvmax,j+1,-1
             if (vei(i,jj)/=0) then  
               if (vei(i,jj)<ncels+1) then ; ii=vei(i,jj) ; a=a+malla(ii,1) ; b=b+malla(ii,2) ; c=c+malla(ii,3) 
               cont=cont+1 ; goto 77 ; else ; goto 77 ; end if
             end if 
           end do 
           goto 77 
        end if
        a=a+malla(ii,1) ; b=b+malla(ii,2) ; c=c+malla(ii,3) ; cont=cont+1
        do jj=1,nvmax ; if (vei(ii,jj)==iii) then ; jjj=jj  ; exit ; end if ; end do
        do jj=jjj+1,nvmax  !comencem la gira
          if (vei(ii,jj)/=0) then ; if (vei(ii,jj)>ncels) goto 77 ; iii=ii ; ii=vei(iii,jj) ; goto 66 ; end if
        end do
        do jj=1,jjj-1  !comencem la gira
          if (vei(ii,jj)/=0) then ; if (vei(ii,jj)>ncels) goto 77 ; iii=ii ; ii=vei(iii,jj) ; goto 66 ; end if
        end do
      end if
77    marge(i,j,1)=a/cont ; marge(i,j,2)=b/cont ; marge(i,j,3)=c/cont 
    end do
  end do
end subroutine calculmarges

subroutine reaccio_difusio
  real*8 pes(ncals,nvmax)         !area del contacte entre i i el vei(i,j)
  real*8 areap(ncals,nvmax)
  real*8 suma,areasota
  real*8 hq3d(ncals,ncz,ng)
  real*8 hq2d(ncals,ngg)
  real*8 ux,uy,uz,dx,dy,dz,ua,ub,uc
  integer primer 

  hq3d=0.
  hq2d=0.

  do i=1,ncels
    pes(i,:)=0. ; areap(i,:)=0.
ui: do j=1,nvmax
      if (vei(i,j)/=0.) then 
        ua=malla(i,1) ; ub=malla(i,2) ; uc=malla(i,3)
        do jj=j+1,nvmax
          if (vei(i,jj)/=0.) then
pes(i,j)=sqrt((marge(i,j,1)-marge(i,jj,1))**2+(marge(i,j,2)-marge(i,jj,2))**2+(marge(i,j,3)-marge(i,jj,3))**2)
            ux=marge(i,j,1)-ua  ; uy=marge(i,j,2)-ub  ;  uz=marge(i,j,3)-uc 
            dx=marge(i,jj,1)-ua ; dy=marge(i,jj,2)-ub ; dz=marge(i,jj,3)-uc
            areap(i,j)=0.05D1*sqrt((uy*dz-uz*dy)**2+(uz*dx-ux*dz)**2+(ux*dy-uy*dx)**2)
            cycle ui
          end if
        end do
pes(i,j)=sqrt((marge(i,j,1)-marge(i,1,1))**2+(marge(i,j,2)-marge(i,1,2))**2+(marge(i,j,3)-marge(i,1,3))**2) 
        ux=marge(i,j,1)-ua ; uy=marge(i,j,2)-ub ; uz=marge(i,j,3)-uc 
        dx=marge(i,1,1)-ua ; dy=marge(i,1,2)-ub ; dz=marge(i,1,3)-uc
        areap(i,j)=0.05D1*sqrt((uy*dz-uz*dy)**2+(uz*dx-ux*dz)**2+(ux*dy-uy*dx)**2)
      end if
    end do ui
    areasota=sum(areap(i,:))
    suma=sum(pes(i,:))+2*areasota ; areasota=areasota/suma ; pes(i,:)=pes(i,:)/suma 
    do k=1,4 !ng ATENCIO
      do kk=2,ncz-1
        hq3d(i,kk,k)=hq3d(i,kk,k)+areasota*(q3d(i,kk-1,k)-q3d(i,kk,k))
        hq3d(i,kk,k)=hq3d(i,kk,k)+areasota*(q3d(i,kk+1,k)-q3d(i,kk,k))
        do j=1,nvmax
          if (vei(i,j)/=0) then 
            ii=vei(i,j)
            if (ii==ncals) then
!              if (k/=1) then     !ACHTUNG aixo es perque als marges de la dent tenim molt activador: ames es calcul ineficient
                hq3d(i,kk,k)=hq3d(i,kk,k)+pes(i,j)*(-q3d(i,kk,k)*0.044D1)      !sink     
!              end if
            else
              hq3d(i,kk,k)=hq3d(i,kk,k)+pes(i,j)*(q3d(ii,kk,k)-q3d(i,kk,k)) 
            end if
          end if
        end do
      end do
      !kk=ncz
      hq3d(i,ncz,k)=areasota*(-q3d(i,ncz,k)*0.044D1) 
      hq3d(i,ncz,k)= hq3d(i,ncz,k)+areasota*(q3d(i,ncz-1,k)-q3d(i,ncz,k))
      do j=1,nvmax
        if (vei(i,j)/=0) then 
          ii=vei(i,j)
          if (ii==ncals) then
!            if (k/=1) then     !ACHTUNG aixo es perque als marges de la dent tenim molt activador
              hq3d(i,ncz,k)=hq3d(i,ncz,k)+pes(i,j)*(-q3d(i,ncz,k)*0.044D1)   !sink     
!            end if
          else
            hq3d(i,ncz,k)=hq3d(i,ncz,k)+pes(i,j)*(q3d(ii,ncz,k)-q3d(i,ncz,k)) 
          end if
        end if
      end do
    end do
    pes(i,:)=pes(i,:)*suma ; areasota=areasota*suma ; suma=suma-areasota ; pes(i,:)=pes(i,:)/suma 
    areasota=areasota/suma
    do k=1,4 !ng ATENCIO
      hq3d(i,1,k)=areasota*(q3d(i,2,k)-q3d(i,1,k))
      do j=1,nvmax
        if (vei(i,j)/=0) then 
          ii=vei(i,j)
          if (ii==ncals) then
!            if (k/=1) then
              hq3d(i,1,k)=hq3d(i,1,k)+pes(i,j)*(-q3d(i,1,k)*0.044D1)     
!            end if
          else
            hq3d(i,1,k)=hq3d(i,1,k)+pes(i,j)*(q3d(ii,1,k)-q3d(i,1,k)) 
          end if
        end if
      end do
    end do

    !aixo es perque no flux al contorn
!    pes(i,:)=pes(i,:)*suma ; areasota=areasota*suma 
!    do j=1,nvmax
!      k=vei(i,j)
!      if (k>ncels) then
!        suma=suma-pes(i,j)
!      end if
!    end do
!    suma=suma-areasota ; pes(i,:)=pes(i,:)/suma 
!    do k=1,ngg
!      do j=1,nvmax
!        if (vei(i,j)/=0) then 
!          ii=vei(i,j)
!          a=q2d(ii,k)
!          if (ii<ncels+1) then   ! atencio condicions de contorn de NO SINK 
!            hq2d(i,k)=hq2d(i,k)+pes(i,j)*(a-q2d(i,k))
!          end if
!        end if
!      end do
!    end do
  end do
  do i=1,4 ! ng ATENCIO
    q3d(:,:,i)=q3d(:,:,i)+delta*difq3d(i)*hq3d(:,:,i)
  end do
!  do i=1,ng
!    a=maxval(q3d(:,:,i))
!    do j=1,10
!      b=j ; b=1/b
!      if (a/b>1) exit
!    end do    
!    b=j ; b=b*1D7
!    q3d(:,:,i)=aint(q3d(:,:,i)*b) ; q3d(:,:,i)=q3d(:,:,i)/b
!  end do
!  do i=1,ngg
!    q2d(:,i)=q2d(:,i)+delta*difq2d(i)*hq2d(:,i)
!  end do

  !REACCIO
  hq3d=0.
  do i=1,ncels
!    areasota=sum(areap(i,:))  ! ATENCIO no ho fem perque es concentracio (cal canviar si al dividir dividim la quantitat)
    if (q3d(i,1,1)>1) then
      if (i>=ncils) knots(i)=1 
    end if
    a=acac*q3d(i,1,1)-q3d(i,1,4)
    if (a<0) a=0.
    hq3d(i,1,1)=a/(1+ihac*q3d(i,1,2))-mu*q3d(i,1,1)
    if (q2d(i,1)>us) then
      hq3d(i,1,2)=q3d(i,1,1)*q2d(i,1)-mu*q3d(i,1,2)
    else
      if (knots(i)==1) then
        hq3d(i,1,2)=q3d(i,1,1)-mu*q3d(i,1,2)
      end if
    end if
    if (q2d(i,1)>ud) then
      a=ih*q2d(i,1)-mu*q3d(i,1,3)
      if(a<0.) a=0.
      hq3d(i,1,3)=a
    else
      if (knots(i)>ud) then
        a=ih-mu*q3d(i,1,3)
        if(a<0.) a=0.
        hq3d(i,1,3)=a
      end if
    end if

      a=acec*q3d(i,1,1)-mu*q3d(i,1,4)-acaca*q3d(i,1,3)
      if(a<0.) a=0.
      hq3d(i,1,4)=a
!    hq3d(i,1,:)=hq3d(i,1,:)*areasota
  end do

  !degradacio
!  hq3d(:,:,1)=hq3d(:,:,1)-mu*q3d(:,:,1)

if (maxval(abs(hq3d(:,1,1:2)))>1D100) then ; 
!print *,"PANIC OVERFLOW" ; 
panic=1 ; return ; end if
  do i=1,4 
    q3d(:,1,i)=q3d(:,1,i)+delta*hq3d(:,1,i)
  end do

  where(q3d<0.) q3d=0.  

end subroutine reaccio_difusio

subroutine diferenciacio
  do i=1,ncels
    q2d(i,1)=q2d(i,1)+tadif*(q3d(i,1,3))
    if (q2d(i,1)>1.) q2d(i,1)=1.
  end do
end subroutine diferenciacio

subroutine empu
  real*8 ux,uy,uz,uux,uuy,uuz,ua,ub,uc,uuuz,uaa,ubb,uuux,uuuy,duux,duuy
  real*8 persu(nvmax,3)
  integer index(nvmax)

  hvmalla=0.
  hmalla=0
  do i=ncils,ncels
    if (knots(i)==1) cycle
    ua=malla(i,1) ; ub=malla(i,2) ; uc=malla(i,3)
    persu=0.
    aa=0 ; bb=0 ; cc=0
    do j=1,nvmax
      k=vei(i,j)
      if (k==0.or.k>ncels) cycle
      b=uc-malla(k,3)
      if (b<-1D-4) then
!        ux=malla(k,1)  ; uy=malla(k,2)  ; uz=malla(k,3)
        uux=ua-malla(k,1)      ; uuy=ub-malla(k,2)      ; uuz=uc-malla(k,3) 
        d=sqrt(uux**2+uuy**2+uuz**2)
        d=1/d
!        if (abs(uux)<1D-13) uux=0. ; if (abs(uuy)<1D-13) uuy=0. ; if (abs(uuz)<1D-13) uuz=0.
        aa=aa-uux*d ; bb=bb-uuy*d ; cc=cc-uuz*d
      end if
    end do
    d=sqrt(aa**2+bb**2+cc**2)
    if (d>0) then
!      a=1!q3d(i,1,2) ; if (a>tadif) a=tadif
      d=tacre/d
!      d=d/(1+tadi*q3d(i,1,1)) 
      a=1-q2d(i,1) ; if (a<0) a=0.
      d=d*a
      hmalla(i,1)=aa*d
      hmalla(i,2)=bb*d
      hmalla(i,3)=cc*d
    end if
  end do

  do i=1,ncils-1
    aa=0. ; bb=0. ; a=-0.3 ; b=0. ; c=0. 
    ua=malla(i,1) ; ub=malla(i,2)
    do j=1,nvmax
      k=vei(i,j)
      if (k<1.or.k>ncels) cycle
      if (k>ncils-1) then 
        uux=ua-malla(k,1)   ; uuy=ub-malla(k,2) 
        d=sqrt(uux**2+uuy**2)
        if (d>0) then
          c=acos(uux/d)
          if (uuy<0) c=2*pii-c !acos(uux/d)
        end if
      else
!        ux=malla(k,1)  ; uy=malla(k,2)  
        uux=ua-malla(k,1)      ; uuy=ub-malla(k,2)
        d=sqrt(uux**2+uuy**2)
        if (d>0) then
        if (a==-0.3) then
          a=acos(uux/d)
          if (uuy<0) a=2*pii-a
          if (d>0) then
            dd=1/d
            uuux=-uuy*dd           ; uuuy=uux*dd
            uaa=acos(uuux)
            if (uuuy<0) uaa=2*pii-uaa
          end if
        else
          b=acos(uux/d)
          if (uuy<0) b=2*pii-b
          if (d>0) then
            dd=1/d
            duux=-uuy*dd           ; duuy=uux*dd
            ubb=acos(duux)
            if (duuy<0) ubb=2*pii-ubb!acos(duux) !ubb
          end if
        end if  
        end if
      end if
    end do

      if (a<b) then ; d=a ; a=b ; b=d ; end if
      if (c<a.and.c>b) then 
        if (uaa<a.and.uaa>b) then ; uuux=-uuux ; uuuy=-uuuy ; end if ! es a la banda de dins i aleshores l'hem d'invertir
        if (ubb<a.and.ubb>b) then ; duux=-duux ; duuy=-duuy ; end if
      else
        if (uaa>a.or.uaa<b) then ; uuux=-uuux ; uuuy=-uuuy ; end if ! es a la banda de dins i aleshores l'hem d'invertir
        if (ubb>a.or.ubb<b) then ; duux=-duux ; duuy=-duuy ; end if 
      end if    
      aa=-uuux-duux ; bb=-uuuy-duuy  

      !ara mirem que sigui cap a fora de la dent a lo cutre
      a=ua+aa ; b=ub+bb
      c=ua-aa ; d=ub-bb
      dd=sqrt(a**2+b**2)
      ddd=sqrt(c**2+d**2)
      if (ddd>dd) then ; aa=-aa ; bb=-bb ; end if

    ! ames tenim la traccio cap abaix deguda a l'adhesio al mesenkima    
    d=sqrt(aa**2+bb**2)
    if (d>0) then         
!      a=tahor*q3d(i,1,3)  !biaix sobre el mesenkima
      d=(d+tahor*q3d(i,1,3))/d
      aa=aa*d  
      bb=bb*d
    end if
    cc=tazmax
    d=sqrt(aa**2+bb**2+cc**2)
    if (d>0) then
!      a=1-q2d(i,1) ; if (a<0) a=0.
      d=tacre/d
!      d=d/(1+tadi*q3d(i,1,1))  
      a=1-q2d(i,1) ; if (a<0) a=0.
      d=d*a                          !aixo sembla estar repe i malament
      hmalla(i,1)=aa*d
      hmalla(i,2)=bb*d
      hmalla(i,3)=cc*d
    end if
  end do

!  do i=1,ncels
!    if (malla(i,1)>crema.or.malla(i,1)<-crema) hmalla(i,1)=0.
!  end do

  hvmalla=hmalla

end subroutine empu

subroutine stelate
  real*8 ax,ay

  !emputja perperdicular?

 ! hvmalla=0.
  ! cal que empu estigui just abans
  do i=1,ncels
    ax=hmalla(i,1) ; ay=hmalla(i,2) 
    d=sqrt(ax**2+ay**2)
    if (d/=0) then
      c=hmalla(i,3)
      if (d>0) then
        a=sqrt(ax**2+ay**2+c**2) ; a=-c/a
        ax=ax*a ; ay=ay*a
        dd=sqrt(ax**2+ay**2+d**2) ; dd=difq2d(2)*q3d(i,1,3)/dd
        if (dd>0) then
        a=1-q2d(i,1) ; if (a<0) a=0.
        ax=ax*dd*a ; ay=ay*dd*a ; d=d*dd*a
        hmalla(i,1)=hmalla(i,1)-ax ; hmalla(i,2)=hmalla(i,2)-ay ;hmalla(i,3)=hmalla(i,3)-d     
        end if
      end if
    end if
  end do

!  hmalla=hmalla+hvmalla

end subroutine stelate

subroutine pushing
  real*8 ux,uy,uz,ua,ub,uc,hx,hy,hz,dd,d,uux,uuy,sux,suy,dr,rd
  real*8 persu(nvmax,3)
  integer index(nvmax)
  !rotllu finite elements
!  hvmalla=0.
  do i=1,ncels
    ua=malla(i,1) ; ub=malla(i,2) ; uc=malla(i,3)
    persu=0.
    do j=1,nvmax
      k=vei(i,j)
      if (k>0.and.k<ncels+1) then
!        ux=malla(k,1) ; uy=malla(k,2) ; uz=malla(k,3)
        ux=malla(k,1)-ua ; uy=malla(k,2)-ub ; uz=malla(k,3)-uc 
        if (abs(ux)<1D-15) ux=0.
        if (abs(uy)<1D-15) uy=0.
        if (abs(uz)<1D-15) uz=0.
        dr=sqrt(ux**2+uy**2+uz**2)
        rd=marge(i,j,5)
        if (dr<1D-8) dr=0.
        if (rd<1D-8) rd=0.
        if (knots(i)==1.and.knots(k)==1) then
          d=dr-rd 
          dr=d/dr 
          persu(j,1)=ux*dr ; persu(j,2)=uy*dr ; persu(j,3)=uz*dr
        else
          if (dr<rd) then
            d=dr-rd 
            dr=d/dr 
            persu(j,1)=ux*dr ; persu(j,2)=uy*dr ; persu(j,3)=uz*dr
          else
            if (i>ncils-1) then
              persu(j,1)=ux*crema ; persu(j,2)=uy*crema ; persu(j,3)=uz*crema 
            end if
          end if
        end if
      end if
    end do

    !ara sumem en ordre
!    c=elas*(1+tahor*q3d(i,1,3))
!    if (c>1) c=1
!    call ordenarepe(abs(persu(:,1)),index,nvmax)
!    a=0. ; do j=1,nvmax ; a=a+persu(index(j),1) ; end do ;
!hmalla(i,1)=hmalla(i,1)+a*c
!    call ordenarepe(abs(persu(:,2)),index,nvmax)
!    a=0. ; do j=1,nvmax ; a=a+persu(index(j),2) ; end do ;
!hmalla(i,2)=hmalla(i,2)+a*c
!    call ordenarepe(abs(persu(:,3)),index,nvmax)
!    a=0. ; do j=1,nvmax ; a=a+persu(index(j),3) ; end do ;
!hmalla(i,3)=hmalla(i,3)+a*c

    !versio rapida sense ordenar (possible biaixos per floats)
    c=elas !*(1+tahor*q3d(i,1,3))
    if (c>1) c=1
    a=0. ; do j=1,nvmax ; a=a+persu(j,1) ; end do ;
hmalla(i,1)=hmalla(i,1)+a*c
    a=0. ; do j=1,nvmax ; a=a+persu(j,2) ; end do ;
hmalla(i,2)=hmalla(i,2)+a*c
    a=0. ; do j=1,nvmax ; a=a+persu(j,3) ; end do ;
hmalla(i,3)=hmalla(i,3)+a*c


  end do

!  hvmalla=hmalla

end subroutine pushing

subroutine pushingnovei
  real*8 ux,uy,uz,ua,ub,uc,hx,hy,hz,dd,d,uux,uuy
  real*8, allocatable :: persu(:,:),cpersu(:,:)   !arbitrari  FACTOR CRITIC DE OPTIMITZACIO
  integer,allocatable :: index(:)
  integer conta,espai,espaia
  espai=20
  allocate(persu(espai,3))
  allocate(index(espai))
  !rotllu finite elements
!  hvmalla=0.
  do i=1,ncels
!    if (q2d(i,1)>=1.) cycle
    ua=malla(i,1) ; ub=malla(i,2) ; uc=malla(i,3)
    persu=0. ; conta=0
gg: do ii=1,ncels
      if (ii==i) cycle 
      do j=1,nvmax ; if (vei(i,j)==ii) cycle gg ; end do
!      ux=malla(ii,1) ; uy=malla(ii,2) ; uz=malla(ii,3)          
      ux=malla(ii,1)-ua 
      if (ux>0.14D1) cycle
      uy=malla(ii,2)-ub 
      if (uy>0.14D1) cycle
      uz=malla(ii,3)-uc
      if (uz>0.14D1) cycle
      if (abs(ux)<1D-15) ux=0.
      if (abs(uy)<1D-15) uy=0.
      if (abs(uz)<1D-15) uz=0.
      d=sqrt(ux**2+uy**2+uz**2)
!      d=aint(d*1D8)*1D-8
      if (d<0.14D1) then
        conta=conta+1
        if (conta>espai) then 
          espaia=espai
          espai=espai+20
          allocate(cpersu(espai,3)) ; cpersu=0.
          cpersu(1:espaia,:)=persu
          deallocate(persu) ; deallocate(index)
          allocate(persu(espai,3)) ; allocate(index(espai))
          persu=cpersu
          deallocate(cpersu)
        end if 
        dd=1/(d+10D-1)**8 ; d=dd/d ; d=aint(d*1D8)*1D-8
        persu(conta,1)=-ux*d ; persu(conta,2)=-uy*d  ; persu(conta,3)=-uz*d 
      end if
    end do gg

    !ara sumem en ordre
!    c=elas*(1+tahor*q3d(i,1,3))
!    if (c>1) c=1
!    call ordenarepe(abs(persu(:,1)),index,espai)
!    a=0. ; do j=1,espai ; a=a+persu(index(j),1) ; end do ;
!hmalla(i,1)=hmalla(i,1)+a*elas
!    call ordenarepe(abs(persu(:,2)),index,espai)
!    a=0. ; do j=1,espai ; a=a+persu(index(j),2) ; end do ;
!hmalla(i,2)=hmalla(i,2)+a*elas
!    call ordenarepe(abs(persu(:,3)),index,espai)
!    a=0. ; do j=1,espai ; a=a+persu(index(j),3) ; end do ;
!hmalla(i,3)=hmalla(i,3)+a*elas
!  end do

    !versio rapida sense ordenar (possible biaixos per floats)
    c=elas !*(1+tahor*q3d(i,1,3))
    if (c>1) c=1
    a=0. ; do j=1,espai ; a=a+persu(j,1) ; end do ;
hmalla(i,1)=hmalla(i,1)+a*elas
    a=0. ; do j=1,espai ; a=a+persu(j,2) ; end do ;
hmalla(i,2)=hmalla(i,2)+a*elas
    a=0. ; do j=1,espai ; a=a+persu(j,3) ; end do ;
hmalla(i,3)=hmalla(i,3)+a*elas
  end do

!  hmalla=hmalla+hvmalla
end subroutine pushingnovei

subroutine biaixbl
  do i=1,ncils-1
    if (malla(i,2)<-tadi) then
      q3d(i,1,1)=bil
    else
      if (malla(i,2)>tadi) then
        q3d(i,1,1)=bib
      end if
    end if
  end do
end subroutine


subroutine promig  !genera biaix
  real*8 n
  real*8 pmalla(ncals,3)

  pmalla=malla
  do i=ncils,ncels
    if (q2d(i,1)==1) cycle
    a=0. ; b=0. ; c=0. ; n=0
    do j=1,nvmax
      k=vei(i,j)
      if (k/=0.and.k<ncels+1) then
        a=a+malla(k,1) ; b=b+malla(k,2) ; c=c+malla(k,3)
        n=n+1
      end if
    end do
    n=1/n
    a=a*n ; b=b*n ; c=c*n 
    a=a-malla(i,1)
    b=b-malla(i,2)
    c=c-malla(i,3)
    pmalla(i,1)=malla(i,1)+delta*radibi*a
    pmalla(i,2)=malla(i,2)+delta*radibi*b
    if (knots(i)==0) then
      a=1-q2d(i,1) ; if (a<0) a=0.
      pmalla(i,3)=malla(i,3)+delta*radibi*c*a
    end if
  end do

  !per als marges
  do i=1,ncils-1
    if (q2d(i,1)==1) cycle
    a=0. ; b=0. ; c=0. ; n=0
    do j=1,nvmax
      k=vei(i,j)
      if (k>0.and.k<ncils.and.k<ncels+1) then
        a=a+malla(k,1) ; b=b+malla(k,2) ; c=c+malla(k,3)
        n=n+1
      end if
    end do
    n=1/n
    a=a*n ; b=b*n ; c=c*n 
    a=a-malla(i,1)
    b=b-malla(i,2)
    c=c-malla(i,3)
    pmalla(i,1)=malla(i,1)+delta*radibi*a
    pmalla(i,2)=malla(i,2)+delta*radibi*b
    if (knots(i)==0) then
      a=1-q2d(i,1) ; if (a<0) a=0.
      pmalla(i,3)=malla(i,3)+delta*radibi*c*a
    end if
  end do
  malla=pmalla
end subroutine promig

subroutine actualitza

!determinem els extrems
  do i=1,ncils-1
    if (abs(malla(i,2))<radibii) then
      if (malla(i,1)>0) then ; hmalla(i,1)=hmalla(i,1)*bia ; 
        hmalla(i,3)=hmalla(i,3)*fac ; 
      end if
      if (malla(i,1)<0) then ; hmalla(i,1)=hmalla(i,1)*bip ; 
        hmalla(i,3)=hmalla(i,3)*fac ; 
      end if
    end if    
  end do

  do i=1,ncels
    if (hmalla(i,3)<0) hmalla(i,3)=0. !es degut a la pressio del stelate
  end do

  do i=1,ncels
    if (knots(i)==1) hmalla(i,3)=0.
  end do
  do i=1,ncels
!    if (q2d(i,1)<1) then
      malla(i,:)=malla(i,:)+delta*hmalla(i,:)  ! a musfinal tadi=0
!    end if
  end do
end subroutine actualitza

subroutine ordenarepe(ma,mt,rang)
  integer rang
  real*8 ma(rang)
  integer mt(rang)
  integer i,j,k
  real*8 a
    mt=0
el: do i=1,rang
      a=ma(i) ; k=1
      do j=1,rang ; if (a>ma(j)) k=k+1 ; end do 
      do j=k,rang ; if (mt(j)==0) then ; mt(j)=i ; cycle el ; end if ; end do
    end do el 
end subroutine ordenarepe

subroutine afegircel

integer primer,segon 

real*8,  allocatable :: cmalla(:,:)  ! les posicions dels nodes x,y,z
integer, allocatable :: cvei(:,:),ccvei(:,:)
integer, allocatable :: cnveins(:)
integer, allocatable :: cknots(:)
real*8,  allocatable :: cq2d(:,:),cmarge(:,:,:),ccmarge(:,:,:)    ! quantitats que son 2d
real*8,  allocatable :: cq3d(:,:,:)    ! quantitats que son 3d act,inh,fgf,ect,p
integer,  allocatable :: scvei(:,:)
integer,  allocatable :: scnveins(:)
real*8 ,  allocatable :: scmalla(:,:)
real*8 ,  allocatable :: scq3d(:,:,:)
real*8 ,  allocatable :: scq2d(:,:)
real*8 ,  allocatable :: scmarge(:,:,:)
integer ,  allocatable :: scknots(:)

integer             up(nvmax),do(nvmax)
real*8 pup(nvmax),pdo(nvmax)
real*8 ua,ub,uc,ux,uy,uz,dx,dy
integer             ord(nvmax)

integer nousnodes(ncels*nvmax,2),externsa(ncels*nvmax)
integer pillats(nvmax),cpillats(nvmax)
integer nncels,ancels
integer cj,ini,fi,sjj,ji,ij,ijj,jji

  nnous=0
  nousnodes=0
  externsa=0

  !primer identifiquem i anomenen els nous nodes i rescalem la matriu malla i vei
  do i=1,ncels
    primer=0. ; kkk=0 ; ji=0
    do j=1,nvmax
      if (vei(i,j)>ncels) then ; ji=1 ; exit ; end if
    end do
    do j=1,nvmax
      k=vei(i,j)
      ua=malla(i,1) ; ub=malla(i,2) ; uc=malla(i,3)
      if (k/=0.and.k>i.and.k<=ncels) then
        ux=malla(k,1) ; uy=malla(k,2) ; uz=malla(k,3)
        ux=ux-ua ; uy=uy-ub ; uz=uz-uc
        a=sqrt(ux**2+uy**2+uz**2)
        a=dnint(a*1D9)*1D-9
        if (a>dmax) then  ! afegim nou node
          nnous=nnous+1 ; nousnodes(nnous,1)=i ; nousnodes(nnous,2)=k
          if (i<ncils.and.k<ncils) then ; externsa(nnous)=1 ; end if
        end if
      end if    
    end do 
  end do

  if (nnous>0) then
    do i=1,ncals
      do j=1,nvmax
        if (vei(i,j)==0) then
          do jj=j,nvmax-1
            vei(i,jj)=vei(i,jj+1)
          end do
        end if
      end do
    end do

    nncels=ncals+nnous

    allocate(cmalla(nncels,3))   ; allocate(cvei(nncels,nvmax))
    allocate(cnveins(nncels))    ; allocate(cq2d(nncels,ngg))   ; allocate(cq3d(nncels,ncz,ng))
    allocate(ccvei(nnous,nvmax)) ; allocate(cknots(nncels))     ; allocate(cmarge(nncels,nvmax,8))

    cmalla=0. ; cvei=0. ; cnveins=0. ; cq2d=0. ; cq3d=0. ; cknots=0 ; cmarge=0.

    do i=1,nnous
      ii=nousnodes(i,1) ; kk=nousnodes(i,2)
!      q3d(ii,:,:)=q3d(ii,:,:)*0.075D1 ;  q2d(ii,:)=q2d(ii,:)*0.075D1
!      q3d(kk,:,:)=q3d(kk,:,:)*0.075D1 ;  q2d(kk,:)=q2d(kk,:)*0.075D1
    end do

    do i=nncels,ncels+nnous+1,-1
      cmalla(i,:)=malla(i-nnous,:)     ; cvei(i,:)=vei(i-nnous,:)
      cnveins(i)=nveins(i-nnous)       ; cq2d(i,:)=q2d(i-nnous,:)
      cq3d(i,:,:)=q3d(i-nnous,:,:)     ; cknots(i)=knots(i-nnous)
      cmarge(i,:,4:8)=marge(i-nnous,:,4:8)
    end do

    cmalla(1:ncels,:)=malla(1:ncels,:)     ; cvei(1:ncels,:)=vei(1:ncels,:) ; cnveins(1:ncels)=nveins(1:ncels)       
    cq2d(1:ncels,:)=q2d(1:ncels,:)
    cq3d(1:ncels,:,:)=q3d(1:ncels,:,:)     ; cknots(1:ncels)=knots(1:ncels) ; cmarge(1:ncels,:,:)=marge(1:ncels,:,:)   

    do i=1,ncels+nnous ; do j=1,nvmax ; if (cvei(i,j)>ncels) cvei(i,j)=nncels ; end do ; end do

    cvei(ncels+nnous+1:nncels,:)=0

    do i=1,nnous
      ii=nousnodes(i,1) ; kk=nousnodes(i,2) ; jj=ncels+i
      cvei(jj,:)=0 ; cvei(jj,1)=ii ; cvei(jj,2)=kk

      a=cmalla(ii,1)+cmalla(kk,1) ; b=cmalla(ii,2)+cmalla(kk,2)
      d=sqrt(a**2+b**2)
      a=a/d ; b=b/d
      d=d/0.20D1
      d=dnint(d*1D10)*1D-10
      a=d*a ; b=d*b
      cmalla(jj,1)=a ; cmalla(jj,2)=b

      cmalla(jj,3)=(malla(ii,3)+malla(kk,3))*0.05D1
      cq3d(jj,:,:)=(cq3d(ii,:,:)+cq3d(kk,:,:))*0.05D1 ; cq2d(jj,:)=(cq2d(ii,:)+cq2d(kk,:))*0.05D1
!      cq3d(jj,:,:)=(cq3d(ii,:,:)+cq3d(kk,:,:))*0.0334D1 ; cq2d(jj,:)=(cq2d(ii,:)+cq2d(kk,:))*0.0334D1
      !lo de 0.333 en comptes de 0.25 es un truco per que abans hem multiplicat els pares per 3/4
    end do
 
!    where(abs(cmalla)<1D-12) cmalla=0.0 ;  where(abs(cq3d)<1D-12) cq3d=0.0 ; where(abs(cq2d)<1D-12) cq2d=0.0

    do i=1,nnous
      ii=nousnodes(i,1) ; kk=nousnodes(i,2)
      jj=ncels+i
      do j=1,nvmax ; if (cvei(ii,j)==kk) then ; cvei(ii,j)=jj ; exit ; end if ; end do
      do j=1,nvmax ; if (cvei(kk,j)==ii) then ; cvei(kk,j)=jj ; exit ; end if ; end do
    end do

    do i=1,nnous
      pillats=0
      ii=nousnodes(i,1) ; kk=nousnodes(i,2)
      jj=ncels+i
      !ara em de mirar quin pare es per a sequir-lo (sera anomenat ini) es el que no tingui nodes externs cap a j+1
      do j=1,nvmax ; if (vei(ii,j)==kk) then ; jjj=j ; exit ; end if ; end do
      kkk=0
      do jjjj=jjj+1,nvmax
        if (vei(ii,jjjj)>0) then
          if (vei(ii,jjjj)<ncels+1) then 
            ini=ii ; fi=kk ; kkk=1 ; exit
          else 
            ini=kk ; fi=ii ; kkk=1 ; exit 
          end if
        end if       
      end do
      if (kkk==0) then
        do jjjj=1,jjj-1
          if (vei(ii,jjjj)>0) then
            if (vei(ii,jjjj)<ncels+1) then 
              ini=ii ; fi=kk ; exit
            else 
              ini=kk ; fi=ii ; exit 
            end if       
          end if
        end do
      end if
      iii=ini
      cj=1 ; pillats(cj)=iii
      !ara busquem el j en el que iii te a jj (la cel de la que busquem els veins)
      do j=1,nvmax
        if (cvei(iii,j)==jj) then ; jjj=j ; exit ; end if
      end do
      !costat j+1(dreta); reseguim el veinatge cap al costat j+1 de iii
      kkk=0
      do j=jjj+1,nvmax ; jji=cvei(iii,j) ; if (jji/=0.and.jji<ncels+nnous+1) then ; iiii=jji 
      kkk=1 ; exit  ; end if ; end do
      if (kkk==0) then !no em trobat el vei i cal repasar de rosca
        do j=1,jjj-1 ; jji=cvei(iii,j) ; if (jji/=0.and.jji<ncels+nnous+1) then ; iiii=jji 
        kkk=1 ; exit  ; end if ; end do
      end if
      cj=cj+1 ; if (cj>nvmax) then ; panic=1 ; return ; end if ; pillats(cj)=iiii
      do j=1,nvmax ; if (cvei(iiii,j)==iii) then ; jjjj=j ; exit ; end if ; end do
88    iii=iiii ; jjj=jjjj ; kkk=0
      do j=jjj+1,nvmax ; jji=cvei(iii,j) ; if (jji/=0.and.jji<ncels+nnous+1) then ; iiii=jji 
      kkk=1 ; exit  ; end if ; end do
      if (kkk==0) then !no em trobat el vei i cal repasar de rosca
        do j=1,jjj-1 ; jji=cvei(iii,j) ; if (jji/=0.and.jji<ncels+nnous+1) then ; iiii=jji 
        kkk=1 ; exit  ; end if ; end do
      end if
      cj=cj+1 ; if (cj>nvmax) then ; panic=1 ; return ; end if ; pillats(cj)=iiii
      do j=1,nvmax ; if (cvei(iiii,j)==iii) then ; jjjj=j ; exit ; end if ; end do
      if (iiii==fi) then !equinocci
        kkk=0
        do kkkk=jjjj+1,nvmax
          if (cvei(iiii,kkkk)/=0.and.kkk==1) then  
            if (cvei(iiii,kkkk)>ncels+nnous) then
              iiii=ini ; kkk=2 ; cj=cj+1 ; exit
            else              
              sjj=kkkk ; kkk=2 ; exit 
            end if
          end if
          if (cvei(iiii,kkkk)/=0.and.kkk==0) then; kkk=1 ; sjj=kkkk ;  end if
        end do
        if (kkk<2) then
          do kkkk=1,jjjj-1
            if (cvei(iiii,kkkk)/=0.and.kkk==1) then
              if (cvei(iiii,kkkk)/=0.and.kkk==1) then  
                if (cvei(iiii,kkkk)>ncels+nnous) then
                  iiii=ini ; cj=cj+1 ; exit
                else              
                  sjj=kkkk ; exit 
                end if
              end if
            end if
            if (cvei(iiii,kkkk)/=0.and.kkk==0) then; kkk=1 ; sjj=kkkk ; end if
          end do
        end if
        jjjj=sjj-1
      end if

      !hem fet tota la volta
      if (iiii==ini) then
        cpillats=0
        if (cj>nvmax) then ; panic=1 ; return ; end if ;
        pillats(cj)=0 ; cj=cj-1
        do jjj=1,cj
          cpillats(cj-jjj+1)=pillats(jjj) 
        end do
        pillats=cpillats
        !ara mirem quins poden ser nodes realment
        jjj=0
        if (cj>nvmax) then ; panic=1 ; return ; end if ;
        do kkk=1,cj
          kkkk=pillats(kkk)
          if (kkkk>ncels.and.kkkk<=ncels+nnous) jjj=jjj+1
        end do
        if (jjj==0) then  !no tenim nous nodes als costats aleshores el creuament es impossible
          ccvei(i,:)=pillats  
          !ccmarge(i,:,:)
          do j=1,nvmax
            k=ccvei(i,j)
            if (k/=0) then
              ii=nousnodes(i,1) ; iiii=nousnodes(i,2)
              if (k/=ii.and.k/=iiii.and.k<ncels+nnous+1) then ! es un als dels que em de posar conexio
                kkkk=0
uu:             do kk=1,nvmax
uuu:              do kkk=1,nvmax
                    if (cvei(k,kk)/=0.and.cvei(k,kk)==pillats(kkk).and.kkkk==1) then ; ji=kk ; exit uu ; end if
                    if (cvei(k,kk)/=0.and.cvei(k,kk)==pillats(kkk).and.kkkk==0) then ; kkkk=1 ; ij=kk ; exit uuu ; end if
                  end do uuu
                end do uu
                !hem de conectar entre ij i ji
                if (ji-ij==1) then
                  do kk=nvmax,ji+1,-1 ; cvei(k,kk)=cvei(k,kk-1) ; cmarge(k,kk,4:8)=cmarge(k,kk-1,4:8) ; end do 
                  cvei(k,ji)=jj
!; cmarge(k,ji,4:5)=0.
                else
                  cvei(k,ji+1)=jj ; 
!cmarge(k,ji+1,4:5)=0.
                end if
              end if
            end if
          end do     
        else                       !el txungu ser a sinomes tenim un nou  perque aleshores hi hauran nomes 3 veins
          ccvei(i,:)=0
          ccvei(i,1)=ini
          kkkk=1
          if (cj>nvmax) then ; panic=1 ; return ; end if ;
rtt:      do kkk=1,cj
            jjjj=pillats(kkk)
            if (jjjj==fi) then 
              kkkk=kkkk+1 ; ccvei(i,kkkk)=fi 
            else
              if (jjjj>ncels) then
                kkkk=kkkk+1 ; ccvei(i,kkkk)=jjjj
              end if
            end if
          end do rtt
goto 899
          do j=1,nvmax
            k=ccvei(i,j)
            if (k/=0) then
              ii=nousnodes(i,1) ; iiii=nousnodes(i,2)
              if (k/=ii.and.k/=iiii.and.k<ncels+1) then ! es un als dels que em de posar conexio
                kkkk=0
uuuu:           do kk=1,nvmax
uuuuu:            do kkk=1,nvmax
                    if (cvei(k,kk)/=0.and.cvei(k,kk)==pillats(kkk).and.kkkk==1) then ; ji=kk ; exit uuuu ; end if
                    if (cvei(k,kk)/=0.and.cvei(k,kk)==pillats(kkk).and.kkkk==0) then ; kkkk=1 ; ij=kk ; exit uuuuu ; end if
                  end do uuuuu
                end do uuuu
                !hem de conectar entre ij i ji
                if (ji-ij==1) then
                  do kk=nvmax,ji+1,-1 ; cvei(k,kk)=cvei(k,kk-1) ; cmarge(k,kk,4:8)=cmarge(k,kk-1,4:8) ; end do
                  cvei(k,ji)=jj
                else
                  cvei(k,ji+1)=jj
                end if
              end if
            end if
          end do
899 continue     
        end if

        !ara cal afegir les conexions a nodes externs
        ii=nousnodes(i,1) ; kk=nousnodes(i,2)
        kkk=0 ; jjj=0
        do j=1,nvmax
          if (cvei(ii,j)>ncels+nnous) then ; kkk=1 ; exit ; end if
        end do
        do j=1,nvmax
          if (cvei(kk,j)>ncels+nnous) then ; kkk=kkk+1 ; exit ; end if
        end do
        if (kkk==2) then
          do j=1,nvmax
            if (ccvei(i,j)==ii) then ; ij=j ; exit ; end if
          end do
          do j=1,nvmax
            if (ccvei(i,j)==kk) then ; jjj=j ; exit ; end if
          end do 
          if (ij>jjj) then 
            ji=ij ; ij=jjj
          else
            ji=jjj
          end if
          !hem de conectar entre ij i ji
          if (ji-ij==1) then
            do kk=nvmax,ji+1,-1 ; ccvei(i,kk)=ccvei(i,kk-1) ; cmarge(i,kk,4:8)=cmarge(i,kk-1,4:8) ; end do 
            ccvei(i,ji)=nncels;
          else
            ccvei(i,ji+1)=nncels ; !cmarge(i,ji+1,4:5)=0.
          end if
        end if 
        cycle 
      end if
      goto 88
    end do

    !ara cal afegir les conexions externes

    !ara remplacem
    cvei(ncels+1:ncels+nnous,:)=ccvei(1:nnous,:)

    deallocate(malla)
    allocate(malla(nncels,3))
    malla=cmalla

    !calculem les noves distancies basals de les noves cels
    do i=ncels+1,ncels+nnous
      ua=malla(i,1) ; ub=malla(i,2) ; uc=malla(i,3)
      do j=1,nvmax
        ii=cvei(i,j)
        if (ii>0.and.ii<ncels+nnous+1) then
          ux=malla(ii,1) ; uy=malla(ii,2) ; uz=malla(ii,3) 
          ux=ux-ua ; uy=uy-ub ; uz=uz-uc
          if (abs(ux)<10D-14) ux=0.
          if (abs(uy)<10D-14) uy=0.
          if (abs(uz)<10D-14) uz=0.
          d=sqrt(ux**2+uy**2)
          cmarge(i,j,5)=d
          d=sqrt(ux**2+uy**2+uz**2)
          cmarge(i,j,4)=d
          cmarge(i,j,6)=ux ;  cmarge(i,j,7)=uy ;  cmarge(i,j,8)=uz 
        end if
      end do
    end do

    !calculem les distancies basals de les noves conexions entre cels velles i noves
    do i=1,ncels
      ua=malla(i,1) ; ub=malla(i,2) ; uc=malla(i,3)
      do j=1,nvmax
        ii=cvei(i,j)
        if (ii>ncels.and.ii<ncels+nnous+1) then
          ux=malla(ii,1) ; uy=malla(ii,2) ; uz=malla(ii,3) 
          ux=ux-ua ; uy=uy-ub ; uz=uz-uc
          if (abs(ux)<10D-14) ux=0.
          if (abs(uy)<10D-14) uy=0.
          if (abs(uz)<10D-14) uz=0.
          d=sqrt(ux**2+uy**2)
          cmarge(i,j,5)=d
          d=sqrt(ux**2+uy**2+uz**2)
          cmarge(i,j,4)=d
          cmarge(i,j,6)=ux ;  cmarge(i,j,7)=uy ;  cmarge(i,j,8)=uz 
        end if
      end do  
    end do  

    ncals=nncels
    ancels=ncels
    ncels=ncels+nnous

    deallocate(vei)    ; deallocate(hmalla)  ; deallocate(hvmalla)
    deallocate(marge) ;  deallocate(nveins) ; deallocate(q2d)     ; deallocate(q3d)
    deallocate(px)    ;  deallocate(py)     ; deallocate(pz)      ; deallocate(knots)

    allocate(vei(nncels,nvmax)) ; allocate(hmalla(nncels,3))  
    allocate(hvmalla(nncels,3))
    allocate(marge(nncels,nvmax,8)) ;  allocate(nveins(nncels))    ; allocate(q2d(nncels,ngg))   
    allocate(q3d(nncels,ncz,ng))
    allocate(px(nncels)) ; allocate(py(nncels)) ; allocate(pz(nncels)) ; allocate (knots(nncels))

    vei=cvei ; nveins=cnveins ; q2d=cq2d ; q3d=cq3d ; knots=cknots ; marge=cmarge ; hmalla=0.
    deallocate(cmalla)  ; deallocate(cvei)    
    deallocate(cnveins) ; deallocate(cq2d)  ; deallocate(cq3d)
    deallocate(ccvei)   ; deallocate(cknots); deallocate(cmarge) 

    !treiem els zeros
    do i=1,ncals
      do j=1,nvmax
        if (vei(i,j)==0) then
          do jj=j,nvmax-1
            vei(i,jj)=vei(i,jj+1)
          end do
        end if
      end do
    end do
    do i=1,ncals
      ii=0
      do j=1,nvmax
        if (vei(i,j)>0) ii=ii+1
      end do
      nveins(i)=ii
    end do
  end if   !end if FINAL SI TENIM NOVES CELS

  do iii=1,nnous
    if (externsa(iii)==1) then  !tenim una nova cel externa
      ii=ancels+iii
      if (ncils==centre) centre=ii
      allocate(scvei(ncels,nvmax))
      allocate(scnveins(ncels))
      allocate(scmalla(ncels,3))
      allocate(scq3d(ncels,ncz,ng))
      allocate(scq2d(ncels,ngg))
      allocate(scmarge(ncels,nvmax,8))
      allocate(scknots(ncels))
      scmalla=malla
      scvei=vei
      scq2d=q2d
      scq3d=q3d
      scnveins=nveins
      scmarge=marge
      scknots=knots
      vei(ii,:)=scvei(ncils,:)
      vei(ncils,:)=scvei(ii,:)
      nveins(ii)=scnveins(ncils)
      nveins(ncils)=scnveins(ii)
      malla(ii,:)=scmalla(ncils,:)
      malla(ncils,:)=scmalla(ii,:)
      q2d(ii,:)=scq2d(ncils,:)
      q2d(ncils,:)=scq2d(ii,:)
      q3d(ii,:,:)=scq3d(ncils,:,:)
      q3d(ncils,:,:)=scq3d(ii,:,:)
      marge(ii,:,:)=scmarge(ncils,:,:)
      marge(ncils,:,:)=scmarge(ii,:,:)
      knots(ii)=scknots(ncils)
      knots(ncils)=scknots(ii)
      scvei=vei

      do i=1,ncels
        do j=1,nvmax
          if (scvei(i,j)==ii) vei(i,j)=ncils
        end do
      end do
      do i=1,ncels
        do j=1,nvmax
         if (scvei(i,j)==ncils) vei(i,j)=ii
        end do
      end do
      deallocate(scvei)
      deallocate(scnveins)
      deallocate(scmalla)
      deallocate(scq3d)
      deallocate(scq2d)
      deallocate(scmarge)
      deallocate(scknots)
      ncils=ncils+1
    end if
  end do


  if (nnous>0) then
    call perextrems
  end if

end subroutine afegircel

subroutine perextrems
 !fa extrems les noves cels que son al marge entre dos extrems per al biaix
 integer,allocatable :: mau(:)
 integer nousn(ncils)
 integer nunous,nmaaa
  !ara coregim els extrems
     nousn=0
     nunous=0
er:  do i=1,ncils-1
       if (i==3.or.i==6) cycle !ATENCIO NO ES ESCALABLE AMB RADI
       kk=0
       do ii=1,nmaa
         iii=mmaa(ii)
         if (iii==i) cycle er
       end do
err:   do j=1,nvmax
         k=vei(i,j)
         if (k<ncils) then
           do ii=1,nmaa
             iii=mmaa(ii)
             if (k==iii) then
               if (kk==1) then
                 nunous=nunous+1
                 nousn(nunous)=i    
                 cycle er
               else
                 kk=1
                 cycle err
               end if
             end if
           end do
         end if
       end do err
     end do er

     if (nunous>0) then
       nmaaa=nmaa
       allocate(mau(nmaa))
       mau=mmaa
       deallocate(mmaa)
       nmaa=nmaa+nunous
       allocate(mmaa(nmaa))
       mmaa(1:nmaa-nunous)=mau
       do i=1,nunous
         mmaa(nmaaa+i)=nousn(i)
       end do 
       deallocate(mau)
     end if

     nousn=0
     nunous=0
era:  do i=1,ncils-1
       if (i==3.or.i==6) cycle !ATENCIO NO ES ESCALABLE AMB RADI
       kk=0
       do ii=1,nmap
         iii=mmap(ii)
         if (iii==i) cycle era
       end do
erra:   do j=1,nvmax
         k=vei(i,j)
         if (k<ncils) then
           do ii=1,nmap
             iii=mmap(ii)
             if (k==iii) then
               if (kk==1) then
                 nunous=nunous+1
                 nousn(nunous)=i    
                 cycle era
               else
                 kk=1
                 cycle erra
               end if
             end if
           end do
         end if
       end do erra
     end do era

     if (nunous>0) then
       nmaaa=nmap
       allocate(mau(nmap))
       mau=mmap
       deallocate(mmap)
       nmap=nmap+nunous
       allocate(mmap(nmap))
       mmap(1:nmap-nunous)=mau
       do i=1,nunous
         mmap(nmaaa+i)=nousn(i)
       end do 
       deallocate(mau)
     end if
end subroutine


subroutine controlz
integer oncz
real*8, allocatable :: cq3d(:,:,:)
  oncz=ncz
  ncz=ncz+1
  !print *,oncz,ncz,"ncz"
  allocate (cq3d(ncals,ncz,ng))
  cq3d=0
  cq3d(:,1:oncz,:)=q3d
  deallocate (q3d)
  allocate(q3d(ncals,ncz,ng))
  q3d=cq3d
  deallocate (cq3d)
!print *,ncz,"control"
end subroutine controlz

subroutine vbia
  real*8 a

  k=0
  do i=1,ncels
    do j=1,ncels    
      if (i==j) cycle
      b=malla(i,1) ; c=malla(j,1)
      if (b<0.and.c>0.or.b>0.and.c<0) then
        c=-c
        a=sqrt((b-c)**2+(malla(i,2)-malla(j,2))**2+(malla(i,3)-malla(j,3))**2)
        if (a<1D-5.and.a>9D-16) then 
          k=1 ; !print *,i,j,a,temps,"dif"
          !print *,malla(i,1),malla(j,1)
          !print *,malla(i,2),malla(j,2)  
          !print *,malla(i,3),malla(j,3)
        end if
      end if
    end do
  end do
!  if (k==1) read(*,*)
end subroutine vbia

subroutine iteracio(tbu)
  integer tbu,ite,io
  do ite=1,tbu
    panic=0
    hmalla=0.
    call reaccio_difusio
    if (panic==1) return
    call biaixbl
    call diferenciacio
    call empu
    call stelate
    call pushingnovei
    call pushing
!    call biaixbld
    call promig
    call actualitza
    call afegircel
    call calculmarges
    temps=temps+1
    open(1,file=nfpro,access='append')
    write (1,*) iteraciototal*(iti-1)+temps
    close(1)
end do
!print *,temps

!a=0
!do i=1,ncels
!  if (malla(i,3)>a) then
!    a=malla(i,3)
!    ii=i
!  end if
!end do
!  print *,temps,maxval(q3d(ncils:ncels,1,1)),"act",maxval(q3d(:,:,2)),"inh"
!print *,ncels,maxval(q2d(:,1))
!  print *,"inh",maxval(q3d(:,:,3)),"fgf",q3d(1,1,3),"marge fgf",q3d(1,1,1),"marge act",q3d(1,1,2),"marge inh"
!j=0 ; a=0.
!do i=1,ncels
!  if (q3d(i,1,1)>a) then ; a=q3d(i,1,1) ; j=i ; end if 
!end do
!print *,j,"inde"
  !print *,maxval(q3d(:,:,4)),"ect"
  !print *,maxval(hmalla(:,1)),"hx",maxval(hmalla(:,2)),"hy",maxval(hmalla(:,3)),"hz",maxval(q2d(:,1)),"dif" 
  !print *,a,"angle"
  !print *,nnous,kko,maxval(nveins),"maxveins",ncz,"ncz",focus,"focus"
end subroutine iteracio

end module coreop2d

!***************************************************************************
!***************  MOQUL ***************************************************
!***************************************************************************
module esclec
use coreop2d
public:: guardaforma,guardaveins,guardapara,llegirforma,llegirveins,llegirpara,llegir
character*50, public :: fifr,fivr,fipr,fifw,fivw,fipw 
integer, public :: nom,map,fora,pass,passs,maptotal,is,maxll
integer, parameter :: mamax=5000
real*8,  public :: mallap(1000,3,mamax)  !atencio si ncels>1000 el sistema peta al llegir
real*8,  public :: parap(30,mamax)
character*50, public :: noms(30)
integer, public :: knotsp(1000,mamax)
integer, public, allocatable :: veip(:,:,:)
real*8, public,allocatable :: ma(:)
real*8, public :: vamax,vamin
character*30, public :: cac,cad,caq,cat,cas,cau,cass
contains
subroutine guardapara
  parap(1,map)=iteraciototal*(iti-1)+temps
  write (2,5)  parap(1:5,map)
  write (2,5)  parap(6:10,map)
  write (2,5)  parap(11:15,map)
  write (2,5)  parap(16:20,map)
  write (2,5)  parap(21:25,map)
  write (2,5)  parap(26:30,map)
  write (2,*)  ncz,ncils
5 format(5F15.6)
end subroutine guardapara

subroutine guardaforma
  write (2,*) temps,ncels
  do i=1,ncels ; write (2,*) malla(i,:) ; end do
end subroutine guardaforma

subroutine guardacon
  write (2,*) ng,ncels
  do i=1,ncels ; do j=1,ncz ; write (2,*) q3d(i,j,:) ; end do ; end do
end subroutine guardacon

subroutine guardaex
  write (2,*) ngg,ncels
  do i=1,ncels ; write (2,*) q2d(i,:) ; end do
end subroutine guardaex

subroutine guardaknots
  write (2,*) temps,ncels
  i=sum(knots)
  write (2,*) i
  do i=1,ncels
    if (knots(i)==1) write (2,*) i
  end do
end subroutine guardaknots

subroutine guardaveins(cvei)
  integer cvei(ncals,nvmax),ccvei(nvmax)
  write (2,*) temps,ncels  
  do i=1,ncels
    k=0 ; ccvei=0
    do j=1,nvmax
      if (cvei(i,j)/=0) then ; k=k+1 ; ccvei(k)=cvei(i,j) ; end if
    end do    
    write (2,*) k
    write (2,*) ccvei(1:k)
  end do
end subroutine guardaveins

subroutine guardamarges(cvei)
  integer cvei(ncals,nvmax)
  real*8 ccmarges(nvmax,8)
  write (2,*) temps,ncels  
  do i=1,ncels
    k=0 ; ccmarges=0
    do j=1,nvmax
      if (cvei(i,j)/=0) then ; k=k+1 ; ccmarges(k,:)=marge(i,j,:) ; end if
    end do    
    write (2,*) k,"cell shape"
    do kk=1,k
      write (2,*) ccmarges(kk,1:3)
    end do
  end do
end subroutine guardamarges

subroutine llegirparatxt
  character*50 cara
  do i=3,30
    read (2,*,END=666,ERR=777)  a,cara ; parap(i,map)=a ; noms(i)=cara !; print *,i,a,cara,"we" 
  end do
5 format(5F13.6)
  Return
889 print *,"fi";return
999 print *,"fi";return
888 print *,"error";return

777 print *,"error de lectura para" ; fora=1 ; close(2) ; return
666 print *,"fi de fitxer para"     ; print *,parap(1:5,map); fora=1 ; close(2) ; return
end subroutine llegirparatxt

subroutine escriuparatxt
  character*50 cara
  do i=3,30
    a=parap(i,map)
    cara=noms(i)
    write (2,*)  a,cara 
  end do
  write (2,*) ncels,"number of cells"
  write (2,*) temps,"number of iterations"
end subroutine escriuparatxt

subroutine llegirpara
print *,"si"
read (*,*)
  read (2,*,END=666,ERR=777)  parap(1:5,map)
  read (2,*,END=666,ERR=777)  parap(6:10,map)
  read (2,*,END=666,ERR=777)  parap(11:15,map)
  read (2,*,END=666,ERR=777)  parap(16:20,map)
  read (2,*,END=666,ERR=777)  parap(21:25,map)
print *,parap
read (*,*)
  read (2,*,END=666,ERR=777)  parap(26:30,map)
5 format(5F13.6)
  return
777 print *,"error de lectura para" ; fora=1 ; close(2) ; return
666 print *,"fi de fitxer para"     ; print *,parap(1:5,map); read (*,*) ; fora=1 ; close(2) ; return
end subroutine llegirpara

subroutine llegirforma
  read (2,*,END=666,ERR=777) temps,ncels
  do i=1,ncels ; read (2,*,END=666,ERR=666) malla(i,:) ; end do
  return
777 print *,"error de lectura forma" ; fora=1 ; close(2) ; return
666 print *,"fi de fitxer forma"     ; fora=1 ; close(2) ; return
end subroutine llegirforma

subroutine llegirex
  read (2,*,END=666,ERR=777) temps,ncels
  do i=1,ncels ; read (2,*,END=666,ERR=666) q3d(i,1,:) ; end do
  return
777 print *,"error de lectura ex" ; fora=1 ; close(2) ; return
666 print *,"fi de fitxer ex"     ; fora=1 ; close(2) ; return
end subroutine llegirex

subroutine llegirknots
  read (2,*,END=666,ERR=777) temps,ncels
  read (2,*,END=666,ERR=777) j
  do i=1,j
    read (2,*,END=666,ERR=777) k
    knots(k)=1 
  end do
  return
777 print *,"error de lectura knots" ; fora=1 ; close(2) ; return
666 print *,"fi de fitxer knots"     ; fora=1 ; close(2) ; return
end subroutine llegirknots

subroutine llegirveins
  read (2,*,END=666,ERR=777) temps,ncels  
  vei=0
  do i=1,ncels 
    read (2,*,END=666,ERR=777) k          
    read (2,*,END=666,ERR=777) vei(i,1:k)  
  end do
  return
777 print *,"error de lectura v" ; fora=1 ; close(2) ; return
666 print *,"fi de fitxer v"     ; fora=1 ; close(2) ; return
end subroutine llegirveins

subroutine agafarparap(imap)
integer imap
  parap(1,imap)=temps ; parap(2,imap)=ncels        
  parap(3,imap)=tacre ; parap(4,imap)=tahor ; parap(5,imap)=elas   ; parap(6,imap)=tadi ; parap(7,imap)=crema
  parap(8,imap)=acac  ; parap(9,imap)=ihac  ; parap(10,imap)=acec ; parap(11,imap)=ih  ; parap(12,imap)=acaca
  do j=1,ng  ; parap(12+j,imap)=difq3d(j)    ; end do
  do j=1,ngg ; parap(12+ng+j,imap)=difq2d(j) ; end do
  parap(17,imap)=us ; parap(18,imap)=ud;
  parap(13+ng+ngg,imap)=bip ; parap(14+ng+ngg,imap)=bia ; parap(15+ng+ngg,imap)=bib ;  parap(16+ng+ngg,imap)=bil
  parap(17+ng+ngg,imap)=radi 
  parap(18+ng+ngg,imap)=mu     ; parap(19+ng+ngg,imap)=tazmax
  parap(20+ng+ngg,imap)=radibi ; parap(14+ng+1,imap)=tadif  ;
  parap(15+ng+1,imap)=fac ; parap(21+ng+ngg,imap)=radibii ; parap(12,imap)=ncals
end subroutine agafarparap

subroutine posarparap(imap)
integer imap
  temps=parap(1,imap) ; ncels=parap(2,imap)
  tacre=parap(3,imap) ; tahor=parap(4,imap) ; elas=parap(5,imap) ; tadi=parap(6,imap)  ; crema=parap(7,imap)
  acac=parap(8,imap)  ; ihac=parap(9,imap) ; acec=parap(10,imap); ih=parap(11,imap)   ; acaca=parap(12,imap)
  do j=1,ng  ; difq3d(j)=parap(12+j,imap)    ; end do
  do j=1,ngg ; difq2d(j)=parap(12+ng+j,imap) ; end do
  us=parap(17,imap) ; ud=parap(18,imap);
  bip=parap(13+ng+ngg,imap) ; bia=parap(14+ng+ngg,imap) ; bib=parap(15+ng+ngg,imap) ; bil=parap(16+ng+ngg,imap)
  radi=parap(17+ng+ngg,imap); mu=parap(18+ng+ngg,imap)  ; tazmax=parap(19+ng+ngg,imap)
  radibi=parap(20+ng+ngg,imap) ; tadif=parap(14+ng+1,imap) 
  fac=parap(15+ng+1,imap) ; radibii=parap(21+ng+ngg,imap) !!!!!!; ncals=parap(12,imap)
end subroutine posarparap

subroutine llegir
  character*50 cac
  if (passs==2) then
    print *,"name of the file (scratch/raw/fill//name) "
    read (*,*) cac
!    open(2,file="/scratch/raw/fillv12.913"//cac,status='old',iostat=i)
    open(2,file=cac,status='old',iostat=i)
    fora=0 ; ki=ncels
    pass=1
    passs=0
    allocate(veip(1000,nvmax,mamax))
  else
    if (pass==0) then
      allocate(veip(1000,nvmax,mamax))
      open(1,file="inex/noms.dad",status='old',iostat=nom)
      fora=0
      pass=1
      ki=ncels
      !print *,nom,"fitxer"
      if (nom/=0) then
        fifw="INmodel/dents.dad"
        fifr="INmodel/dents.dad"
      else
        read(1,*) fifw
        read(1,*) fifr 
      end if
      close(1)
!      open(2,file=fifr,status='old',iostat=nom)
      open(2,file=cac,status='old',iostat=i)
    end if
  end if
  mallap=0
  parap=0
  veip=0.
  knotsp=0 
  mallap=0.
  do map=1,mamax
    ki=ncels
    call llegirparatxt
    call posarparap(map)
    if (fora==1) exit
    if (ncels/=ki) then
      ncals=ncels
      deallocate (malla) ; deallocate(vei) ; deallocate(knots)
      allocate(malla(ncels,3)) ; allocate(vei(ncels,nvmax)) ; allocate(knots(ncels))
      vei=0 ; malla=0 ; knots=0
    end if
    call llegirveins
    veip(1:ncels,:,map)=vei
    if (fora==1) exit
    call llegirknots
    knotsp(1:ncels,map)=knots
    if (fora==1) exit
    call llegirforma
    mallap(1:ncels,:,map)=malla
    if (fora==1) exit
  end do
  maxll=map-1
  map=1
  call posarparap(map)
  deallocate (malla) ;  deallocate(q3d)  ; deallocate(px)    ; deallocate(py)     ; deallocate(pz) 
  deallocate (q2d)   ;  deallocate(marge); deallocate(vei)   ; deallocate(nveins) ; deallocate(hmalla)
  deallocate(hvmalla);  deallocate(knots) ; deallocate(mmaa) ; deallocate (mmap)
  ncals=ncels
  call redime
  knots=knotsp(1:ncels,map)           
  malla=mallap(1:ncels,:,map)
  vei=veip(1:ncels,:,map)
  !determinemt ncils
  ncils=0
  do i=1,ncels
    do j=1,nvmax
      if (vei(i,j)>=ncals) then ; ncils=ncils+1 ; exit ; end if
    end do
  end do
  ncils=ncils+1
  arrow_key_func=0 ![PASSAR]
  pass=1
end subroutine llegir

subroutine llegirinicial
open(2,file=cac,status='old',iostat=i)
map=1
call llegirparatxt
call posarparap(map)
close(2)
end subroutine llegirinicial

subroutine guardaveinsoff(cvei)
  integer cvei(ncals,nvmax),ccvei(nvmax)
  real*8 c(4),mic(4)
    !em de passar de veinatge a faces

  integer ja(ncels*6,4)

  integer face(ncels*20,5)
  integer nfa(ncels*20)
  integer nfaces
  integer pasos(10)
  integer npasos,bi,nop
  real*8 mamx

  integer nfacre

  allocate(ma(ncels))
    call mat

  nfaces=0
  nfacre=0

    do i=1,ncels
ale: do j=1,nvmax
       bi=0
       ii=vei(i,j) ; if (ii==0.or.ii>ncels) cycle
ele:   do k=1,nvmax
         iii=vei(ii,k) ; if (iii==0.or.iii>ncels.or.iii==i) cycle
         do kk=1,nvmax
           iiii=vei(iii,kk) ; if (iiii==0.or.iiii>ncels) cycle
           if (iiii==i) then !triangle trobat
             nfaces=nfaces+1
             !write (2,*) nfaces,i,ii,iii
            ! do jj=1,nfacre
            !   if (ja(jj,1)==i.and.ja(jj,2)==ii.and.ja(jj,3)==iii) goto 7891
            !   if (ja(jj,1)==ii.and.ja(jj,2)==iii.and.ja(jj,3)==i) goto 7891
            !   if (ja(jj,1)==iii.and.ja(jj,2)==ii.and.ja(jj,3)==i) goto 7891
            !   if (ja(jj,1)==iii.and.ja(jj,2)==i.and.ja(jj,3)==ii) goto 7891
            !   if (ja(jj,1)==ii.and.ja(jj,2)==i.and.ja(jj,3)==iii) goto 7891
            !   if (ja(jj,1)==i.and.ja(jj,2)==iii.and.ja(jj,3)==ii) goto 7891
            ! end do
		! nfacre=nfacre+1
            ! ja(nfacre,1)=i
            ! ja(nfacre,2)=ii
            ! ja(nfacre,3)=iii
7891         bi=bi+1
             nop=iii     
             if (bi==1) cycle ele
             cycle ale
           end if
         end do
       end do ele
      
       do k=1,nvmax
         iii=vei(ii,k) ; if (iii==0.or.iii>ncels.or.iii==i.or.iii==nop) cycle
         if (bi==0) cycle
         ! a per els quadrats
         do kk=1,nvmax
           iiii=vei(iii,kk) ; 
           if (iiii==0.or.iiii>ncels.or.iiii==ii.or.iiii==nop) cycle
           do kkk=1,nvmax
             jj=vei(iiii,kkk)
             if (jj==i) then !triangle trobat
               nfaces=nfaces+1
               !write (2,*) nfaces,i,ii,iii,iiii
               cycle ale
             end if
           end do
         end do
       end do

    end do ale
  end do
  
  write (2,*) "COFF"  
  write (2,*) ncels,nfaces,ncels
  write (2,*) " "
  do i=1,ncels ; call get_rainbow(ma(i),vamin,vamax,c) ; write (2,*) malla(i,:),c ; end do

nfaces=0
ja=0
nfacre=0

  write (2,*) " "  
  !write (2,*) "faces"  
    do i=1,ncels
aale: do j=1,nvmax
       bi=0
       ii=vei(i,j) ; if (ii==0.or.ii>ncels) cycle
aele:   do k=1,nvmax
         iii=vei(ii,k) ; if (iii==0.or.iii>ncels.or.iii==i) cycle
         do kk=1,nvmax
           iiii=vei(iii,kk) ; if (iiii==0.or.iiii>ncels) cycle
           if (iiii==i) then !triangle trobat
               nfaces=nfaces+1
           !  do jj=1,nfacre
           !    if (ja(jj,1)==i.and.ja(jj,2)==ii.and.ja(jj,3)==iii) goto 789
           !    if (ja(jj,1)==ii.and.ja(jj,2)==iii.and.ja(jj,3)==i) goto 789
           !    if (ja(jj,1)==iii.and.ja(jj,2)==ii.and.ja(jj,3)==i) goto 789
           !    if (ja(jj,1)==iii.and.ja(jj,2)==i.and.ja(jj,3)==ii) goto 789
           !    if (ja(jj,1)==ii.and.ja(jj,2)==i.and.ja(jj,3)==iii) goto 789
           !    if (ja(jj,1)==i.and.ja(jj,2)==iii.and.ja(jj,3)==ii) goto 789
           !  end do
		! nfacre=nfacre+1
            ! ja(nfacre,1)=i
            ! ja(nfacre,2)=ii
            ! ja(nfacre,3)=iii
               call get_rainbow(ma(i),vamin,vamax,c)
               mic=c ; mamx=ma(i)
               call get_rainbow(ma(ii),vamin,vamax,c)
               mic=mic+c
               !if (ma(ii)>mamx) then ; mic=c ; mamx=ma(ii) ; end if
               call get_rainbow(ma(iii),vamin,vamax,c)
               mic=mic+c
               !if (ma(iii)>mamx) then ; mic=c; end if 
               mic=mic/3.
             write (2,67) 3,i-1,ii-1,iii-1 !,mic
67 format (4I4,4F10.6)
789          bi=bi+1
             nop=iii          
             if (bi==1) cycle aele
             cycle aale
           end if
         end do
       end do aele
      
       do k=1,nvmax
         iii=vei(ii,k) ; if (iii==0.or.iii>ncels.or.iii==i.or.iii==nop) cycle
         if (bi==0) cycle
         ! a per els quadrats
         do kk=1,nvmax
           iiii=vei(iii,kk) ; 
           if (iiii==0.or.iiii>ncels.or.iiii==ii.or.iiii==nop) cycle
           do kkk=1,nvmax
             jj=vei(iiii,kkk)
             if (jj==i) then !triangle trobat
               nfaces=nfaces+1
               call get_rainbow(ma(i),vamin,vamax,c)
               mic=c ; mamx=ma(i)
               call get_rainbow(ma(ii),vamin,vamax,c)
               mic=mic+c 
               !if (ma(ii)>mamx) then ; mic=c ; mamx=ma(ii) ; end if 
               call get_rainbow(ma(iii),vamin,vamax,c)
               mic=mic+c ; 
               !if (ma(iii)>mamx) then ; mic=c ; mamx=ma(iii) ; end if 
               call get_rainbow(ma(iiii),vamin,vamax,c)
               mic=mic+c ; 
               !if (ma(iiii)>mamx) then ; mic=c ; mamx=ma(iiii) ; end if 
               mic=c/4.
               write (2,68) 4,i-1,ii-1,iii-1,iiii-1 !,mic
68 format (5I4,4F10.6)
               cycle aale
             end if
           end do
         end do
       end do

    end do aale
  end do
deallocate(ma)
end subroutine guardaveinsoff

subroutine mat
  ma=0
      do i=1,ncels
        if (knots(i)==1) then ; ma(i)=1.0
        else
          if (q2d(i,1)>us) ma(i)=0.1
          if (q2d(i,1)>ud) ma(i)=1.0
        end if
      end do
  vamax=maxval(ma)
  vamin=minval(ma)
end subroutine

subroutine get_rainbow(val,minval,maxval,c)
real*8, intent(in) :: val,maxval,minval
real*8, intent(out) :: c(4)

real*8 :: f

if (maxval > minval) then
   f = (val-minval)/(maxval-minval)
else ! probably maxval==minval
   f = 0.5
endif

if (f < .07) then
   c(1) = 0.6
   c(2) = 0.6
   c(3) = 0.6
   c(4) = 0.8
elseif (f < .2) then
   c(1) = 1.0
   c(2) = f
   c(3) = 0.0
   c(4) = 0.5
elseif (f < 1.0) then
   c(1) = 1.0
   c(2) = f*3
   c(3) = 0.0
   c(4) = 1.0
else
   c(1) = 1.0
   c(2) = 1.0
   c(3) = 0.0
   c(4) = 1.0
endif

end subroutine get_rainbow

end module esclec


!---------------------------------------------------------------------------

!***************************************************************************
!***************  PROGRAMA           ****************************************
!***************************************************************************

program tresdac

!useopengl_gl
!useopengl_glut
!use view_modifier
!use function_plotter
use coreop2d
use esclec
implicit none

integer :: winid, menuid
integer pa,nstep
real*8    step
integer   sstep,nt,pao
real*8    parapo(30)
character*12 cu,cd,ct,cq
character*12 acu,acd,act,acq
character*1 ccu,ccd,cct,ccq
character*2 dcu,dcd,dct
character*6 dcq
character*31 nff
character*31 nf,nfoff,nfes
character*4 chq
character*6 chs
character*8 chv
character*2 di
character*1 diu
integer paras(19)
integer sis,siss
integer ancels

integer idi,pau,pad,pauu,padd

paras(1)=3 ; paras(2)=4 ;paras(3)=5 ;paras(4)=7 ;paras(5)=8 ;
paras(6)=9 ; paras(7)=11 ;paras(8)=13 ;paras(9)=14 ;paras(10)=15 ;
paras(11)=17 ; paras(12)=18 ;paras(13)=19 ;paras(14)=20 ;paras(15)=21 ;
paras(16)=23 ; paras(17)=27 ;paras(18)=28 ;paras(19)=29 

call getarg(1,cac) !fitxer entrada
call getarg(2,cau) !fitxer sortida
call getarg(3,cad) !iteracions
!iteraciototal=dnum(cad)
!call getarg(4,caq) !parametre a mutar
!call getarg(5,cat) !tan per cert a canviar en cada pas
!call getarg(6,cas) !tan per cert a canviar en cada pas
!sstep=dnum(cas)
call getarg(4,cass) !numero de steps
!step=dnum(cat)
!pa=dnum(caq)
!nstep=dnum(cas)
!print *,cac,cau,cad,cass
open(4,file="kk")
write (4,*) cad,cass
!print *,cad,cass
close(4)
open(4,file="kk")
read (4,*) iteraciototal,sstep
!print *,iteraciototal,sstep
close (4)

if (sstep<0) then ; siss=-1 ; else ;siss=1 ;end if

ncz=4
call ciinicial
call llegirinicial
call dime
!call referci

!parapo=parap(:,1)
!Programa
!parap(:,1)=parapo
  ancels=ncels
  call posarparap(1)
  ncels=ancels
  do iti=1,sstep 
    ii=0
    write (ct,*) (idi+ii)*sis
    write (cq,*) iti*iteraciototal
 
    acu=adjustl(cu)
    acd=adjustl(cd)
    act=adjustl(ct)
    acq=adjustl(cq)

    dcu=acu ; dcd=acd ;dct=act ;dcq=acq

    nff=dcq//"_"//cau

    do i=1,len(nff)
      if (nff(i:i)==" ") nff(i:i)="_"
    end do

    nfpro=cau
    do i=1,len(nfpro)
      if (nfpro(i:i)==" ") nfpro(i:i)="_"
    end do


    nf=nff(1:26)//"_"//".dad"
    nfoff=nff(1:26)//"_"//".off"
    nfes=nff(1:26)//"_"//".txt"
    nfpro=nfpro(1:15)//"_progressbar"//".txt"

!    call referci

    temps=0

    nt=iteraciototal !*iti
    call iteracio(nt)

    di=trim(adjustl(dcu))

    !escritura
    open(2,file=nf,iostat=nom)
    call agafarparap(1)
    call guardapara
    call guardaveins(vei)
    call guardamarges(vei)
    call guardaknots
    call guardaforma
    call guardacon
    call guardaex
    close(2)

    open(2,file=nfoff,iostat=nom)
    call guardaveinsoff(vei)
    close(2)

    open(2,file=nfes,iostat=nom)
    call posarparap(1)
    call escriuparatxt
  end do
!end do
!end do

parap(:,1)=parapo
call posarparap(1)

!end do
!end do

777 print *,"fora"
end program tresdac

!***************************************************************************
!***************  FI  PROGRAMA      ****************************************
!***************************************************************************

