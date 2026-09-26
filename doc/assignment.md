Popis:

    Napište program dns, který bude filtrovat dotazy typu A směřující na domény v rámci dodaného seznamu a jejich poddomény. Ostatní dotazy bude přeposílat v nezměněné podobě specifikovanému resolveru. Odpovědi na dříve přeposlané dotazy bude program předávat původnímu tazateli. Analýza a sestavení DNS zpráv musí být implementována přímo v programu dns. Stačí uvažovat pouze komunikaci pomocí UDP a dotazy typu A. Na jiné typu dotazů a nežádoucí dotazy odpovídejte vhodnou chybovou zprávou.

    Při vytváření programu je povoleno použít pouze knihovny pro práci se sokety a další obvyklé funkce používané v síťovém prostředí (jako je netinet/*, sys/*, arpa/* apod.), knihovnu pro práci s vlákny (pthread), signály, časem, stejně jako standardní knihovnu jazyka C (varianty ISO/ANSI i POSIX), C++ a STL. Jiné knihovny nejsou povoleny.

    Spuštění aplikace
    Použití: dns -s server [-p port] -f filter_file

    Pořadí parametrů je libovolné. Popis parametrů:

        -s: IP adresa nebo doménové jméno DNS serveru (resolveru), kam se má zaslat dotaz.
        -p port: Číslo portu, na kterém bude program očekávat dotazy. Výchozí je port 53.
        -f filter_file: Jméno souboru obsahující nežádoucí domény.

    Podporované typy 
    Uvažujte pouze dotazy typu A, protokol UDP a libovolné protokoly nižších vrstev podporované OS. Není požadována podpora DNSSEC.
    Výstup aplikace

    Program nebude vypisovat žádné informace. Volitelně však můžete implementovat parametr -v (verbose), při jehož uvedení bude program vypisovat informace o překladu ve vámi zvoleném formátu.

    Formát souboru se seznamem nežádoucích domén

    Nežádoucí domény budou dopředu uloženy v lokálním textovém ASCII souboru, každá doména bude uvedena na samostatném řádku. Prázdné řádky a řádky začínající znakem '#' ignorujte. Uvažujte konce řádků používané v OS GNU/Linux, Microsoft Windows i Apple Mac OS.

    Příklad souboru s nežádoucími doménami:

        https://pgl.yoyo.org/adservers/serverlist.php?hostformat=nohtml&showintro=1


    Doplňující informace k zadání

        Před odevzdáním projektu si důkladně pročtěte společné zadání pro všechny projekty.
        Jakékoliv rozšíření nezapomeňte zdůraznit v souboru README a v dokumentaci. Není však možné získat více bodů, než je stanovené maximum.
        Program se musí vypořádat s chybnými vstupy.
        Veškeré chybové výpisy vypisujte srozumitelně na standardní chybový výstup.
        Pokud máte pocit, že v zadání není něco specifikováno, popište v dokumentaci vámi zvolené řešení a zdůvodněte, proč jste jej vybrali.
        V dokumentaci popište, jaké řešení jste zvolili pro procházení seznamu blokovaných domén.
        V dokumentaci uveďte, jaké chybové zprávy váš program generuje a za jakých okolností.
        Vytvořený kód by měl být modulární a otestovaný. Testy, které jste při řešení projektu napsali se spustí voláním "make test".
        Pište robustní aplikace, které budou na vstupu vstřícné k drobným odchylkám od specifikace.
        Při řešení projektu uplatněte znalosti získané v dřívějších kurzech týkající se jak zdrojového kódu (formátování, komentáře), pojmenování souborů, tak vstřícnosti programu k uživateli.

    Referenční prostředí pro překlad a testování
    Program by měl být přenositelný. Referenční prostředí pro překlad budou servery eva.fit.vutbr.cz a merlin.fit.vutbr.cz (program musí být přeložitelný a funkční na obou systémech). Vlastní testování může probíhat na jiném počítači s nainstalovaným OS GNU/Linux, či FreeBSD, včetně jiných architektur než Intel/AMD, jiných distribucí, jiných verzí knihoven apod. Pokud vyžadujete minimální verzi knihovny (dostupné na serveru merlin a eva), jasně tuto skutečnost označte v dokumentaci a README.

    Doporučená literatura

        RFC1035
