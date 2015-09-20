///Hiba változó
unsigned short ERROR = 0;

/**
* Sensor Result Type. A méretétől függ, hogy mennyi szenzort 'tehetünk' az autóra.
* Elég itt megváltoztatni a típust például unsigned short-ra és máris 16 szenzort tudnánk kezelni
*/
typedef uint8_t srtype;
/**
* volatile: akár egy másik alkalmazás is megváltoztathatja
* => interruptoknál muszáj volatile változókat használni
*/
///a szenzorok összegzett értéke egy uchar-ban, minden bit egy szenzort jelent
volatile srtype sensor_results = 0;

/**
* Az arduinoban kivannak kapcsolva a try/catch/throw kulcsszavak így kell valami
* alternatíva.
* a TRY/CATCH/AFTERR macrókat csak a loop()-on belül lehet használni, ugyanis a
* loop()-ot fogja meghívni a THROW macró miután beállította az err értéket
*/
#define TRY						\
if(ERROR == 0)

/**
* AFTERR blokk akkor fut le, ha történt hiba.
*/
#define AFTERR					\
if(ERROR != 0)

/**
* CATCH block a különböző hibaüzeneteket segít megkülönböztetni amennyiben szükséges
*/
#define CATCH(err) 				\
else if(ERROR == err)

/**
* ROLLBACK blokk ha vissza akarunk térni oda ahol tartottunk (mégse hiba, vagy tudjuk máshogy kezelni)
*/
#define ROLLBACK				\
ERROR = 0;						\
return

/**
* a THROW() makró segít kilépni a kontextusból úgy hogy meg hívja a loop() függvényt
* fontos hogy könnyen végtelen rekurzióba kerülünk ha az AFTERR blokkból hívjuk meg
* avagy ha THROW(0)-t dobunk.
*/
#define THROW(errval)			\
ERROR = errval;					\
loop()

/**
* Error értékek
*/
///egy listába csak 255 darab lista elem lehet, ez reális ha csak 98304 byte SRAM van.
#define OUT_OF_BOUNDS 1000
///listában figyelünk hogy ne próbáljunk indirekciót hívni egy null pointerre (azonnal lefagy az arduino)
#define NULL_POINTER 1500
///kimenetből akartunk olvasni, vagy bemenetre írni
#define BAD_PIN_CONFIG 2000
///motornak nem 0 és 1 közötti törtet adtunk
#define BAD_PERCENTAGE 2500
///ha rosszul kezeljük le a szenzorok által szolgáltatott eredményeket
#define BAD_PROCESS 500
///ha az autó úgy érzi elakad, ezt a 'kivételt' dobja
#define I_STUCK_EXCEPTION 3000

/**
* Egy dedikált pin szám a hibát jelző LED-nek
*/
#define ERROR_LED_PIN 23

/**
* Minimum duty cycle a motornak (amire még elindul a kocsi)
*/
#define ENGINE_MIN_DC 200

/**
* A lehető leggyorsabb fokozat a tranzisztor folyamatosan nyitva lesz
*/
#define ENGINE_MAX_DC 255

/**
* A szenzorokkal nem akarjuk túlterhelni az arduinot hogy esetlegesen sűrűn kérnek interruptot
*/
#define SENSOR_DELAY 500

/**
* Tény, hogy nem a legszebb megoldás de az attachInterrupt csak 
* void (*) () paramétert vesz át és ez a megoldás annyiból szebb,
* hogy csak a macrót kell hívogatni, hogy függvényeket generáljunk
* illetve a hozzájuk tartozó idő guard változót is legenerálja.
* Az srtype méretétől (most 8 bit) függ, hogy hány ilyen rutint
* lehet létrehozni, nekünk most 3 is elég.
* ##: preprocessor concatenáció
* @param nth a függvény id-ja illetve hogy a sensor_results \n
* hanyadik bitjéért felelős
* 
* kommentelt példa:
*	volatile unsigned long __isr_0_lockout = 0;
*	void __isr_0_routine() {
*		///aktuális idő (a millis() nem növekszik interrupton belül!)
*		unsigned long c_time = millis();
*		///Ha SENSOR_DELAY-nyin belül újra kért interruptot, gyorsan visszatérünk
* 		if(c_time - __isr_0_lockout < SENSOR_DELAY) return;
*		///ahogy a neve mondja egy 2^n értéket csinálunk
* 		srtype power_of_two = 1;
*		///ha kétszer XOR-ozunk akkor visszakapjuk az eredeti sensor_resultot
*		sensor_results ^= power_of_two;
*		///elmentjük a lockoutba az aktuális időt
*		__isr_0_lockout = c_time;
*	}
*/
#define MAKE_ISR(nth) 											\
volatile unsigned long __isr_##nth##_lockout = 0;				\
void __isr_##nth##_routine() {									\
	unsigned long c_time = millis();							\
	if(c_time - __isr_##nth##_lockout < SENSOR_DELAY) return;	\
	srtype power_of_two = 1;									\
	power_of_two <<= nth;										\
	sensor_results ^= power_of_two;								\
	__isr_##nth##_lockout = c_time;								\
}

/**
* Hogy a függvény neveket se nekünk kelljen kezelni
*/
#define PASS_ISR(nth) 			\
__isr_##nth##_routine

/**
* Lista sablon osztály
* Egyszeresen láncolt elején strázsált iterátoros generikus lista
* Használat:
*  {
*		List<T> minta_lista;
*		minta_lista += new T(...);
*		for(List<T>::Iterator iter = minta_lista.begin(); iter != NULL; ++iter) {
*			///a Type -ból Type *-ot csinál mert csak pointereket tárol
*			T * ptr = *iter;
*		}
*  } ///itt megsemmisül a minta_lista és az összes általa tárolt elem is.
*/
template<class Type> class List{
private:
	/**
	* Listán belüli adatszerkezet
	*/
	struct Item{
		///következő elemre mutató pointer
		Item * next;
		///aktuális adat pointerét tároló elem
		Type * data;
		///alapértelmezett konstruktor
		Item(Type * d = NULL) : next(NULL), data(d) { }
		///destruktor törli az elemeket
		~Item() {
			delete data;
		}
	};

	///strázsa a sor elején
	Item * _tail;
	///ennek a segítségével tartjuk meg a sorrendet
	Item ** _next_ptr;
	///hány elemet tárolunk a listában
	uint8_t _amount;
public:
	/**
	* paraméter nélküli konstruktor
	* inicializáljuk az elemeket
	*/
	List() : _tail(new Item()), _amount(0) {
		///a next_ptr-t beállítjuk
		this->_next_ptr = &this->_tail->next;
	}
	
	/**
	* Szokásos láncolt lista destruktor
	*/
	~List() {
		Item * iter = _tail;
		while(iter) {
			///elmentjük a next-et
			Item * temp = iter->next;
			///letöröljük az aktuális elemet
			delete iter;
			///visszaállítjuk
			iter = temp;
		}
	}

	/**
	* Vissza adja az első nem strázsa elemet (lehet NULL is)
	*/
	Item * begin() const {
		return this->_tail->next;
	}

	/**
	* Bár az index operátort elkészítettem, ne ezt használd hogy bejárd az elemeket
	* Mert egy 20 elemű listánál 0+1+2+...+19 = 190 felesleges iteráció történik amíg
	* elérünk először a 0. elemig majd az 1. elemig majd a 2. elemig stb..
	*/
	Type * operator[](uint8_t index) {
		if(index >= this->_amount) {
			THROW(OUT_OF_BOUNDS);
		}
		uint8_t tmp_i = 0;
		for(Iterator iter = this->begin(); iter != NULL; ++iter) {
			///tudjuk, hogy nem lehet túlindexelni, a felső feltétel mellett mindenképpen lesz találat
			if(tmp_i++ == index) { return *iter; }
		}
	}

	/**
	* Hozzáad egy új elemet a listához
	*/
	List<Type> & operator+=(Type * ptr) {
		///255 elem bőven elég, tekintve, hogy csak 98304 byte hely van.
		if(this->_amount == 0xFF) {
			THROW(OUT_OF_BOUNDS);
		}
		Item * node = new Item(ptr);
		++this->_amount;
		///Tehát, az új elem next pointerét beállítjuk a next_ptr által tárolt pointerre
		node->next = *this->_next_ptr;
		///majd a next_ptr által tárolt pointert átállítjuk node-ra
		*this->_next_ptr = node;
		///majd a next_ptr-re rá referáljuk a next-et
		this->_next_ptr = &node->next;
		return *this;
	}

	/**
	* Egyirányú iterátor osztály csak prefixes operator++-al.
	*/
	class Iterator{
	private:
		///aktuális elemre mutató ptr
		Item * _payload;
	public:
		/**
		* nem const copy ctor
		*/
		Iterator(Item * first_item) : _payload(first_item) { }

		/**
		* Szokásos 'getter' operátor
		*/
		Type * operator*() {
			if(this->_payload == NULL) {
				THROW(NULL_POINTER);
			}
			return this->_payload->data;
		}

		/**
		* Prefixes ++
		*/
		Iterator & operator++() {
			///egy biztonsági null check
			if(this->_payload != NULL) {
				this->_payload = this->_payload->next;
			}
			return *this;
		}

		/**
		* pointereket összehasonlító operator!=
		* @return bool igaz ha nem egyeznek a pointerek
		*/
		bool operator!=(Item * ptr) {
			return this->_payload != ptr;
		}
	};
};

class Part {
private:
	///konstans pin érték
	const uint8_t _pin;
	///konstans pin mód, nem akarjuk futásidőben váltogatni az in-out-ot
	const uint8_t _mode;

	/**
	* ellenőrzi hogy megfelelő-e a típusa az elemnek
	* @param mode_to_match[unsigned char] INPUT | OUTPUT értékek ellen tesztelünk
	*/
	void _check_config(uint8_t mode_to_match) {
		if(this->_mode != mode_to_match) {
			THROW(BAD_PIN_CONFIG);
		}
	}
protected:
	/**
	* Csak a gyerekek által használható construktor
	* @param pin ezen a pin-en keresztül fog kommunikálni a külvilággal a gyerek
	* @param mode az irányát adja meg
	*/
	Part(uint8_t pin, uint8_t mode) : _pin(pin), _mode(mode) {
		///rögtön be is állítjuk az elemet
		pinMode(pin, mode);
	}

	/**
	* Arduino által adott analogWrite fv-re egy wrapper
	* @param duty_cycle 0-255, mennyire legyen kitöltve a PWM
	* átlag Volt számítás: 3.3/256 * duty_cycle = X [V]
	*/
	void analog_write(uint8_t duty_cycle) {
		this->_check_config(OUTPUT);
		analogWrite(this->_pin, duty_cycle);
	}

	/**
	* Arduino által adott digitalWrite(HIGH|LOW)-ra egy wrapper
	* @param value LOW vagy HIGH
	*/
	void digital_write(uint8_t value) {
		this->_check_config(OUTPUT);
		digitalWrite(this->_pin, value);
	}

	/**
	* Arduino által adott analogRead fv-re egy wrapper
	* @return uint8_t mekkora értéket adott az előre beállított pin-ről
	*/
	uint8_t analog_read() {
		this->_check_config(INPUT);
		return analogRead(this->_pin);
	}

	/**
	* Arduino által adott digitalRead fv-re egy wrapper
	* @return uint8_t HIGH|LOW értéket adhat vissza, de mivel egy arduino due-ről fog futni,\n
	* úgyis interrupt service routine-al fogjuk megoldani
	*/
	uint8_t digital_read() {
		this->_check_config(INPUT);
		return digitalRead(this->_pin);
	}
public:
	/**
	* Absztrakt osztályt szeretnénk, és minden egységbe implementálni egy olyan függvényt
	* ami alaphelyzetbe állítja az adott egységet
	*/
	virtual void terminate() = 0;
};

/**
* InterruptService osztály
* Egy keret az arduino által adott attachInterrupt, detachInterrupt függvényekre
*/
class InterruptService {
private:
	///statikus változó ami az összes interrupt service-re kihat
	static bool _enabled;
	///örökre beleragadó pin
	const uint8_t _pin;
	///egy bool érték segítségével követjük hogy detacheltük vagy attacheltük már
	bool _isr_attached;
public:
	/**
	* Az adott pint megjegyzi az osztály
	* @param pin melyik pin kér interruptot
	*/
	InterruptService(uint8_t pin) : _pin(pin), _isr_attached() { }

	/**
	* Ráakasztjuk az interrupt rutint a pin-re ha még nincs ráakaszva
	* @param isr_mode melyik 'event'-re legyen érzékeny a pin: (LOW|HIGH|CHANGE|RISING|FALLING)
	* @see https://www.arduino.cc/en/Reference/AttachInterrupt
	*/
	void attach_interrupt(uint8_t isr_mode, void (*isr) ()) {
		if(!this->_isr_attached) {
			attachInterrupt(this->_pin, isr, isr_mode);
			this->_isr_attached = true;
		}
	}

	/**
	* Kiüti a ráakasztott rutint ha még nincs kiütve
	*/
	void detach_interrupt() {
		if(this->_isr_attached) {
			detachInterrupt(this->_pin);
			this->_isr_attached = false;
		}
	}
	/**
	* Arduino szinten kikapcsolja az interruptokat
	*/
	static void disable() {
		///ha esetleg a loop()-ban hívogatnánk
		if(InterruptService::_enabled) {
			noInterrupts();
			InterruptService::_enabled = false;
		}
	}
	/**
	* Arduino szinten bekapcsolja az interruptokat
	* Időzítésre érzékeny blokkokat lehet képezni a disable()-el együtt
	* @see disable()
	*/
	static void enable() {
		///ha esetleg a loop()-ban hívogatnánk
		if(!InterruptService::_enabled) {
			interrupts();
			InterruptService::_enabled = true;
		}
	}
};
///szeretjük hogy nem lehet inline megoldani
bool InterruptService::_enabled = true;

/**
* LED osztály
* Egy LED-et kezelő osztály
*/
class LED : public Part{
	///LED aktuális állása, logikai értékeknek megfelelő: 0:OFF 1-255:ON
	uint8_t _state;
public:
	/**
	* Szokásos pin számot bekérő ctor
	*/
	LED(uint8_t pin) : Part(pin, OUTPUT), _state(0) { }

	/**
	* Megcseréli a LED állapotát
	*/
	void toggle() {
		///ha most 1ben van, akkor az fv lefutása után 0ban lesz
		if(this->_state == 1) {
			this->digital_write(LOW);
		} else {
			this->digital_write(HIGH);
		}
		this->_state = !this->_state;
	}
	///egyszerűen egy HIGH jelet rakunk a pin-jére
	void switch_on() {
		this->digital_write(HIGH);
	}

	///egyszerűen egy LOW jelet rakunk a pin-jére
	void switch_off() {
		this->digital_write(LOW);
	}

	/**
	* A LED nem veszélyes de azért kiütjük
	*/
	void terminate() {
		this->digital_write(LOW);
	}
};

/**
* Sensor osztály
* egy szenzor elemet reprezentál.
*/
class Sensor : public Part, InterruptService {
	/**
	* Minden szenzornak van egy infra vörös LED-je
	*/
	LED * _ir_led;
public:
	/**
	* kötelező paramétereket evő konstruktor
	* @param pin melyik számú pin lesz az input interfésze a Sensor-nak
	* @param func egy void (*) () prototípusú függvény pointer ami kell az arduino attachInterrupt fv-nek
	* @param ir_led az infra vörös LED-nek a dedikált pin száma
	*/
	Sensor(uint8_t pin, void (* func) (), uint8_t ir_led) : Part(pin, INPUT), InterruptService(pin), _ir_led(new LED(ir_led)) {
		this->attach_interrupt(CHANGE, func);
	}

	/**
	* Kitörli a LED példányt és kikapcsolja az interruptot
	*/
	~Sensor() {
		this->detach_interrupt();
		delete this->_ir_led;
	}

	/**
	* A szenzor input így nem tud túl sok kárt tenni a kocsinak, de az interruptja képes érdekes dolgokra ha nem figyelünk
	*/
	void terminate() {
		this->detach_interrupt();
	}
};

/**
* ServoUnit osztály
* Egy szervót reprezentál és a interfészét fogja magába
*/
class ServoUnit : public Part{
	///aktuális fokszám
	uint8_t _duty_cycle;
public:
	/**
	* Egyszerű konstruktor ami beállítja az őse paramétereit megfelelően
	* @param pin a szervónak dedikált pin
	*/
	ServoUnit(uint8_t pin) : Part(pin, OUTPUT), _duty_cycle() {}

	void terminate() {
		///középre állítás
	}
};

/**
* Engine osztály
* A motort irányítja
*/
class Engine : public Part {
	///a minimum duty cycle amit a motor tranzisztorának állítunk $(később ki lehet ütni és kicserélni a macrokra, ENGINE_MI_DC, ENGINE_MAX_DC)
	const uint8_t _min_dc;
	///a maximum duty cycle amit a motor tranzisztorának állíthatunk
	const uint8_t _max_dc;
public:
	/**
	* @param pin pin ID
	* @param min_dc minimum duty cycle a tranzisztornak
	* @param max_dc maximum duty cycle a tranzisztornak
	*/
	Engine(uint8_t pin, uint8_t min_dc = ENGINE_MIN_DC, uint8_t max_dc = ENGINE_MAX_DC) : Part(pin, OUTPUT), _min_dc(min_dc), _max_dc(max_dc) {}

	/**
	* minimum sebességre állítja a motort
	*/
	void min_speed() {
		this->analog_write(this->_min_dc);
	}

	/**
	* maximum sebességre állítjuk a motort
	*/
	void max_speed() {
		this->analog_write(this->_max_dc);
	}

	/**
	* Bizonyos százalékba állítja a motort
	* @param percentage tört érték 0 és 1között
	* [FONTOS] A 0% nem feltétlenül a megállás hanem a _min_dc érték!
	*/
	void utilize(double percentage) {
		if(percentage > 1.0 || percentage < 0.0) {
			THROW(BAD_PERCENTAGE);
		}
		///adott % szorozva a min és max közötti különbséggel + eltolva a minimummal
		uint8_t dc = (uint8_t)(percentage * (this->_max_dc - this->_min_dc) + this->_min_dc);
		this->analog_write(dc);
	} 
	/**
	* lenullázzuk a kimenő feszültséget
	*/
	void terminate() {
		this->analog_write(0);
	}
};

class Car{
	///LED-ek listája
	List<LED> * _leds;
	///Szenzorok listája
	List<Sensor> * _sensors;
	///Szervó egység
	ServoUnit *_servo;
	///Motor
	Engine * _engine;
public:
	/**
	* Inicializálunk a konstruktorban
	* csak a paraméter nélkülieket az inicializáló listában a jobb átláthatóság érdekében
	*/
	Car(uint8_t servo_pin, uint8_t engine_pin) : _leds(new List<LED>()), _sensors(new List<Sensor>()) {
		this->_servo = new ServoUnit(servo_pin);
		this->_engine = new Engine(engine_pin);
	}
	/**
	* destruktor amiben először leállítunk minden egységet aztán kitöröljük azokat
	*/
	~Car() {
		this->stop();
		delete this->_leds;
		delete this->_sensors;
		delete this->_engine;
		delete this->_servo;
	}

	/**
	* Megállítja az összes elemet fontossági sorrendben kezdve
	*/
	void stop() {
		this->_engine->terminate();
		this->_servo->terminate();
		for(List<Sensor>::Iterator iter = this->_sensors->begin(); iter != NULL; ++iter) {
			(*iter)->terminate();
		}
		for(List<LED>::Iterator iter = this->_leds->begin(); iter != NULL; ++iter) {
			(*iter)->terminate();
		}
	}
	/**
	* Hozzáad egy LED-et az autóhoz
	*/
	void operator+=(LED * ptr) {
		(*this->_leds) += ptr;
	}
	/**
	* Hozzáad egy Sensor-t az autóhoz
	*/
	void operator+=(Sensor * ptr) {
		(*this->_sensors) += ptr;
	}

	/**
	* Feldolgozza a szenzorok által szolgáltatott jelet
	* Itt már sajnos hard kódolt logikát kell alkalmazni a
	* kocsin lévő fizikai pozíciójuk függvényében
	* @param values az érték ahol 1 bit 1szenzornak felel meg
	* @see srtype
	*/
	void process_results(srtype values) {
		switch(values) {
			case 0:
				///semmit se érzékel az autó, megyünk előre
				this->_engine->utilize(1.0);
				break;
			case 7:
				///nem tudunk semerre menni
				this->_engine->terminate();
				THROW(I_STUCK_EXCEPTION);
				break;
			default:
				THROW(BAD_PROCESS);
				break;
		}
	}
};

/**
* Preprocesszorral 'példányosítunk' 3 függvényt.
* - sokkal kevesebb helyet foglal
* - jól skálázható, elég egy typedef-et módosítani, hogy több szenzort tudjunk lekezelni (most 8at tudunk)
* - nincs belőle copy-paste hiba
* @see MAKE_ISR
*/
MAKE_ISR(0)
MAKE_ISR(1)
MAKE_ISR(2)

Car * car;
LED error_led(ERROR_LED_PIN);

void setup() {
	///ha éppen gépnél teszteljük jó ha tudjuk olvasni mit küld esetlegesen
	Serial.begin(9600);
	///létrehozunk egy új kocsi példányt
	car = new Car(9, 12);
	/**
	* Miért kell indirekció a car objektumra? Mert egyébként a pointeréhez akarnánk hozzáadni a másik
	* pointer értékét.
	* 2 megoldás van:
	*  	1.: car->operator+=(new LED(...))
	* 	2.: *car += new LED(...)
	* 	(igazán perverzeknek van több is: (*car).operator+=(new LED(...)))
	*/
	*car += new LED(10);
	/**
	* Hozzáadunk szenzorokat
	* @see PASS_ISR
	*/
	*car += new Sensor(37, PASS_ISR(0), CHANGE);
	*car += new Sensor(39, PASS_ISR(1), CHANGE);
	*car += new Sensor(41, PASS_ISR(2), CHANGE);
}

void loop() {
	/**
	* A try catch afterr blokkok félrevezetőek lehetnek, mert a try makrót itt nem
	* csak arra használjuk fel, hogy a benne lévő függvényeket védjük, hanem ez csak
	* akkor fut le ha a program futása során nem történt hiba
	*/
	TRY {
		///feldolgozzuk a keletkezett adatokat az interruptok során
		car->process_results(sensor_results);
	} CATCH(BAD_PROCESS) {
		///azért szólunk a gépnek, hogy tesztelésnél mégis kiderüljük
		Serial.println("BAD PROCESS ERROR OCCOURED");
		///visszaállítjuk a szenzor értékeket nullába
		sensor_results = 0;
		///és folytatjuk onnan ahonnan 'dobtuk' a hibát
		ROLLBACK;
	} CATCH(I_STUCK_EXCEPTION) {
		///szólunk hogy beakadtunk ha esetleg tesztelnénk
		Serial.println("I STUCK");
		///villogtatjuk a LED-et. Az interrupt úgyis bele fog szólni ha már úgy érzi a robi hogy nincs beakadva
		error_led.toggle();
		delay(ERROR);
		///az elakadás emberi beavatkozást igényel, de semmiképpen sem végzetes probléma
		ROLLBACK;
	} AFTERR {
		///disable interrupt service routines
		InterruptService::disable();
		delete car;
		while(true) {
			error_led.toggle();
			Serial.println(ERROR);
			delay(ERROR);
		}
	}
}
