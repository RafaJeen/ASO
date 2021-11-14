#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rafael Morera");
MODULE_DESCRIPTION("LKM que gestiona diferents GPIOs");
MODULE_VERSION("0.1");

static unsigned int gpioLED1 = 20;
static unsigned int gpioLED2 = 16;
static unsigned int gpioButton1 = 26;
static unsigned int gpioButton2 = 19;
static unsigned int gpioButton3 = 13;
static unsigned int gpioButton4 = 21;
static unsigned int irqNumber1;
static unsigned int irqNumber2;
static unsigned int irqNumber3;
static unsigned int irqNumber4;
static unsigned int numberPressesB1 = 0;
static unsigned int numberPressesB2 = 0;
static bool	    led1On = 0;
static bool	    led2On = 0;


static irq_handler_t  ebbgpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

/** Funcio que inicialitza el LKM
 *  Funcio encarregada de configurar els diferents GPIOs i IRQs
 *  @return retorna 0 si tot ha anat correctament
 */
static int __init ebbgpio_init(void){
   int result = 0;
   printk(KERN_INFO "GPIO_TEST: Initializing the GPIO_TEST LKM\n");

   //Comprovem que els GPIOs son valids (si els que s'han escollit estan disponibles)
   if (!gpio_is_valid(gpioLED1) || !gpio_is_valid(gpioLED2)){
      printk(KERN_INFO "GPIO_TEST: invalid LED GPIO\n");
      return -ENODEV;
   }

   // Configurem el LED1
   led1On = false;                            // Forçem que estigui apagat per defecte
   gpio_request(gpioLED1, "sysfs");           // Fem la peticio a la que s'ha hardcodejat el GPIO del LED
   gpio_direction_output(gpioLED1, led1On);   // Configurem el GPIO perque estigui en mode output i apagat
   gpio_export(gpioLED1, false);              // Fem que el GPIO aparegui a /sys/class/gpio


   //Configurem el LED2
   led2On = false;                            // Forçem que estigui apagat per defecte
   gpio_request(gpioLED2, "sysfs");           // Fem la peticio a la que s'ha hardcodejat el GPIO del LED
   gpio_direction_output(gpioLED2, led2On);   // Configurem el GPIO perque estigui en mode output i apagat
   gpio_export(gpioLED2, false);              //  Fem que el GPIO aparegui a /sys/class/gpio


   //Configurem el button1 (encarregat d'encendre el LED1)
   gpio_request(gpioButton1, "sysfs");       // Configurem el boto
   gpio_direction_input(gpioButton1);        // Configurem el boto per que sigui un input
   gpio_set_debounce(gpioButton1, 200);      // Fem un control de rebots amb un temps de 200ms
   gpio_export(gpioButton1, false);          // Fem que el GPIO aparegui a /sys/class/gpio


   //Configurem el button2 (encarregat d'apagar el LED1)
   gpio_request(gpioButton2, "sysfs");       // Configurem el boto
   gpio_direction_input(gpioButton2);        // Configurem el boto per que sigui un input
   gpio_set_debounce(gpioButton2, 200);      // Fem un control de rebots amb un temps de 200ms
   gpio_export(gpioButton2, false);          // Fem que el GPIO aparegui a /sys/class/gpio


   //Configurem el button3 (encarregat d'encendre el LED2)
   gpio_request(gpioButton3, "sysfs");       // Configurem el boto
   gpio_direction_input(gpioButton3);        // Configurem el boto per que sigui un input
   gpio_set_debounce(gpioButton3, 200);      // Fem un control de rebots amb un temps de 200ms
   gpio_export(gpioButton3, false);          // Fem que el GPIO aparegui a /sys/class/gpio


   //Configurem el button4 (encarregat d'apagar el LED2)
   gpio_request(gpioButton4, "sysfs");       // Configurem el boton
   gpio_direction_input(gpioButton4);        // Configurem el boto per que sigui un input
   gpio_set_debounce(gpioButton4, 200);      // Fem un control de rebots amb un temps de 200ms
   gpio_export(gpioButton4, false);          // Fem que el GPIO aparegui a /sys/class/gpio


   // Missatges per comprovar que els diferents components treballen correctament quan es carrega el LKM
   printk(KERN_INFO "GPIO_TEST: The button1 state is currently: %d\n", gpio_get_value(gpioButton1));
   printk(KERN_INFO "GPIO_TEST: The button2 state is currently: %d\n", gpio_get_value(gpioButton2));
   printk(KERN_INFO "GPIO_TEST: The button3 state is currently: %d\n", gpio_get_value(gpioButton3));
   printk(KERN_INFO "GPIO_TEST: The button4 state is currently: %d\n", gpio_get_value(gpioButton4));

   // Mostrem com s'han mapejat els diferents GPIOs
   irqNumber1 = gpio_to_irq(gpioButton1);
   printk(KERN_INFO "GPIO_TEST: The button2 is mapped to IRQ: %d\n", irqNumber1);

   irqNumber2 = gpio_to_irq(gpioButton2);
   printk(KERN_INFO "GPIO_TEST: The button is mapped to IRQ: %d\n", irqNumber2);

   irqNumber3 = gpio_to_irq(gpioButton3);
   printk(KERN_INFO "GPIO_TEST: The button is mapped to IRQ: %d\n", irqNumber3);

   irqNumber4 = gpio_to_irq(gpioButton4);
   printk(KERN_INFO "GPIO_TEST: The button is mapped to IRQ: %d\n", irqNumber4);


   // Sol·licitem les diferents linies d'interrupcions
   result = request_irq(irqNumber1,             // El numero d'interrupcio que es sol·licita
                        (irq_handler_t) ebbgpio_irq_handler, // Indiquem quina funcio s'ha d'executar quan arribi la interrupcio
                        IRQF_TRIGGER_RISING,
                        "ebb_gpio_handler",
                        NULL);

    result = request_irq(irqNumber2,             // El numero d'interrupcio que es sol·licita
                         (irq_handler_t) ebbgpio_irq_handler, // Indiquem quina funcio s'ha d'executar quan arribi la interrupcio
                         IRQF_TRIGGER_RISING,
                         "ebb_gpio_handler",
                         NULL);

   result = request_irq(irqNumber3,             // El numero d'interrupcio que es sol·licita
                        (irq_handler_t) ebbgpio_irq_handler, // Indiquem quina funcio s'ha d'executar quan arribi la interrupcio
                        IRQF_TRIGGER_RISING,
                        "ebb_gpio_handler",
                        NULL);

    result = request_irq(irqNumber4,             // El numero d'interrupcio que es sol·licita
                         (irq_handler_t) ebbgpio_irq_handler, // Indiquem quina funcio s'ha d'executar quan arribi la interrupcio
                         IRQF_TRIGGER_RISING,
                         "ebb_gpio_handler",
                         NULL);

   printk(KERN_INFO "GPIO_TEST: The interrupt request result is: %d\n", result);
   return result;
}

/** Funcio que neteja el LKM
 *  Funcio encarregada de netejar tots els GPIOs i IRQ i mostrar diferents missatges informatius
 */
static void __exit ebbgpio_exit(void){

   printk(KERN_INFO "GPIO_TEST: The button1 was pressed %d times\n", numberPressesB1);
   printk(KERN_INFO "GPIO_TEST: The button2 was pressed %d times\n", numberPressesB2);

   gpio_set_value(gpioLED1, 0);              // Apaguem el LED1
   gpio_unexport(gpioLED1);                  // Netegem el GPIO del LED1
   gpio_set_value(gpioLED2, 0);              // Apaguem el LED2
   gpio_unexport(gpioLED2);                  // Netegem el GPIO del LED2

   //Alliberem els IRQ
   free_irq(irqNumber1, NULL);
   free_irq(irqNumber2, NULL);
   free_irq(irqNumber3, NULL);
   free_irq(irqNumber4, NULL);

   //Netegem els GPIOs dels botons
   gpio_unexport(gpioButton1);
   gpio_unexport(gpioButton2);
   gpio_unexport(gpioButton3);
   gpio_unexport(gpioButton4);

   //Alliberem els diferents GPIOs
   gpio_free(gpioLED1);
   gpio_free(gpioLED2);

   gpio_free(gpioButton1);
   gpio_free(gpioButton2);
   gpio_free(gpioButton3);
   gpio_free(gpioButton4);

   printk(KERN_INFO "GPIO_TEST: Goodbye from the LKM!\n");
}

/** Funcio encarregada de gestionar les interrupcions
 *  @param irq    El numero IRQ associat al GPIO que ha causat la interrupcio
 *  @param dev_id ID del dispositiu que ha causat la interrupcio
 *  @param regs   h/w valors de registres especifics
 *  return reotrna IRQ_HANDLED si ha anat tot correctament -- per la resta de casos retorna IRQ_NONE.
 */
static irq_handler_t ebbgpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){

  //Configuracio per poder executar els scripts que mostraran diferents missatges informatius
  int err;
  char *argv1[] = { "/home/pi/fase1/Scripts/button1.sh", NULL };
  char *argv2[] = { "/home/pi/fase1/Scripts/button2.sh", NULL };
  char *argv3[] = { "/home/pi/fase1/Scripts/button3.sh", NULL };
  char *argv4[] = { "/home/pi/fase1/Scripts/button4.sh", NULL };

  static char *envp[] = {"HOME=/", "TERM=linux", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };

   if(irq == irqNumber1){
     if(!led1On){ //Controlem que no estigui ences ja el LED (en aquest cas no volem que succeexi res)
       led1On = true;
       gpio_set_value(gpioLED1, led1On);          // Encenem el LED
       numberPressesB1++;
       printk(KERN_INFO "GPIO_TEST: Interrupt! (button1 state is %d)\n", gpio_get_value(gpioButton1));
       printk(KERN_INFO "HE APRETADO %d veces y el LED esta a %d\n", numberPressesB1, led1On);                      // Informem de quants cops s'ha apretat aquest boto

       err = call_usermodehelper( argv1[0], argv1, envp, UMH_NO_WAIT );  //Executem un script per mostrar un missatge informatiu
     }
   } else if(irq == irqNumber2){ //Controlem que no estigui apagat ja el LED (en aquest cas no volem que succeexi res)
     if(led1On){
       led1On = false;
       gpio_set_value(gpioLED1, led1On);          // Apaguem el LED
       printk(KERN_INFO "GPIO_TEST: Interrupt! (button2 state is %d)\n", gpio_get_value(gpioButton2));
       err = call_usermodehelper( argv2[0], argv2, envp, UMH_NO_WAIT ); //Executem un script per mostrar un missatge informatiu
     }
   } else if(irq == irqNumber3){
     if(!led2On){ //Controlem que no estigui ences ja el LED (en aquest cas no volem que succeexi res)
       led2On = true;
       gpio_set_value(gpioLED2, led2On);          // Encenem el LED
       numberPressesB2++;
       printk(KERN_INFO "GPIO_TEST: Interrupt! (button3 state is %d)\n", gpio_get_value(gpioButton3));
       printk(KERN_INFO "HE APRETADO %d veces y el LED esta a %d\n", numberPressesB2, led2On);                      // Informem de quants cops s'ha apretat aquest boto

       err = call_usermodehelper( argv3[0], argv3, envp, UMH_NO_WAIT ); //Executem un script per mostrar un missatge informatiu
     }
   } else if(irq == irqNumber4){
     if(led2On){ //Controlem que no estigui apagat ja el LED (en aquest cas no volem que succeexi res)
       led2On = false;
       gpio_set_value(gpioLED2, led2On);          // Apaguem el LED
       printk(KERN_INFO "GPIO_TEST: Interrupt! (button4 state is %d)\n", gpio_get_value(gpioButton4));

       err = call_usermodehelper( argv4[0], argv4, envp, UMH_NO_WAIT ); //Executem un script per mostrar un missatge informatiu
     }
   }
   return (irq_handler_t) IRQ_HANDLED;      // Indiquem que la IRQ s'ha gestionat correctament
}

// Callbacks que indiquen quines funcions s'han d'executar quan s'inicialitzi / desintal·li el modul
module_init(ebbgpio_init);
module_exit(ebbgpio_exit);
