RCbot
============

A 2015-ös I C What you did last summer BME-s programozós konferencián általam(**Musa Bence**) és **Pongrácz Ádám** által készített és
bemutatott, RC Robot névre hallgató projektnek egy része. Ebben a repo-ban csak maga a beágyazott program van, amit az arduino kártya futtat.

Az előadás diákat megtalálod a **slides** mappában, de magukban nem mondanak túl sokat, így ha érdekel a kontextus akkor olvasd el a mellette levő script.txt fájt.

Fontos tudnivalók
-----------

Ez **nem** egy zárt projekt. Ha be szeretnél csatlakozni akár ebbe, akár egy másik projektünkbe nyugodtan írj egy emailt a **musa.bence@gmail.com** címre. Mint említettem több minden is van már tervben, robotkartól a melegszendvics sütő roboton át egészen az autonóm RC helikopterig. A legnagyobb akadály inkább az, hogy mindegyik irány elég költséges, így elég mélyen át kell gondolni a következő lépést.

Általános
-----------

- Arduino Due
  - Harvard Architektúrájú
  - 96KByte (98304 byte) SRAM
  - 512KByte Flash memória program részére
  - 84MHz 32bites ARM Cortex M3-as CPU
  - DMA vezérlő
  - 3.3V-s logikai magas feszültség (több helyen gond akadt ebből)
  - Bármelyik pinje interruptolható (kivételesen hasznos tulajdonság)

- Érdekességek
  - Több mint 2hónap ment bele az egész projektbe (eddig)
  - Összesen több mint 4000 sor kód, amiből csak 700 az arduinóé, a többi a teszter keretrendszer

GYIK
-----------

- A teszter keretrendszert miért töltöd fel?
  - Nem merem felvállalni, összecsaptam és már van specifikáció egy következőre ahol értelmesebben lesz megoldva. Most jó így is egyenlőre

- Milyen előfeltételekkel csatlakozhatok?
  - Nem várunk el semmi fajta extra tudást (például én 2es voltam digitális technikából). Amit viszont nagyon fontosnak tartunk, hogy elkötelezett legyél.

- Milyen elvárásaitok vannak?
  - Legyen mindig a suli az első. Csak akkor foglalkozz a projekktel ha van rá időd / kedved / hangulatod. Nem kell sietni nem időre megy.

Köszönetnyílvánítás
-----------
Köszönjük Kárpáti Barbarának(PR), Czirkos Zoltánnak, Fehér Bélának és Raikovich Tamásnak azt a sok segítséget amit nyújtottak. Nélkülük nem jutottunk volna el eddig a projekktel.
