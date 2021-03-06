#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//**********************************************************************
// variables compartidas

const int num_prod = 3, num_cons = 4;

const int num_items = 40 ,   // número de items
  tam_vec = 5;   // tamaño del buffer

int producidos = 0, consumidos = 0;

int contador = 0;

unsigned  cont_prod[num_items] = {0}, // contadores de verificación: producidos
  cont_cons[num_items] = {0}, // contadores de verificación: consumidos
  buf[tam_vec] = {0};

Semaphore puede_prod(tam_vec), puede_cons(0); // para controlar que no se lea con el buffer vacío ni se escriba con el buffer lleno
Semaphore acceso(1);

mutex mtx_output, mtx_consumidos, mtx_producidos;
//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato()
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   mtx_output.lock();
   cout << "producido: " << contador << endl << flush ;
   mtx_output.unlock();
   
   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   mtx_output.lock();
   cout << "                  consumido: " << dato << endl ;
   mtx_output.unlock();
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." ;
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  if ( cont_prod[i] != 1 )
      {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora(  )
{

  int dato;
  
  while(true)
   {
     mtx_producidos.lock();
     if(producidos >= num_items){
       mtx_producidos.unlock();
       break;
     }
     
     producidos++;
     mtx_producidos.unlock();

     dato = producir_dato();
    
     sem_wait(puede_prod);
       
     sem_wait(acceso);
     buf[contador++] = dato;
     sem_signal(acceso);

     sem_signal(puede_cons);
   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(  )
{
  int dato;
  
  while(true)
    {
      mtx_consumidos.lock();
      if(consumidos >= num_items){
	mtx_consumidos.unlock();
	break;
      }
      consumidos++;
      mtx_consumidos.unlock();
       
      sem_wait(puede_cons);

      sem_wait(acceso);
      dato = buf[--contador];
      sem_signal(acceso);
       
      sem_signal(puede_prod);
     
      consumir_dato( dato ) ;
    }
}

//----------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución LIFO)." << endl
        << "--------------------------------------------------------" << endl
        << flush ;

   thread hebra_productora[num_prod], hebra_consumidora[num_cons];

   int i,j;
   
   for(i = 0; i < num_prod; i++)
     hebra_productora[i] = thread(funcion_hebra_productora);

   for(j = 0; j < num_cons; j++)
     hebra_consumidora[j] = thread(funcion_hebra_consumidora);

   for(i = 0; i < num_prod; i++)
     hebra_productora[i].join();

   for(j = 0; j < num_cons; j++)
     hebra_consumidora[j].join();

   test_contadores();
}
