/*-------------------------------------------------------------------------------+
|   ~+         *       +                                                         |
|      '                  |     _   _  ___________ _____ _   _ _   _  _____      |
|  ()    .-.,="``"=.    - o -  | \ | ||  ___| ___ \_   _| | | | \ | ||  ___|     |
|        '=/_       \     |    |  \| || |__ | |_/ / | | | | | |  \| || |__       |
|     *   |  '=._    |         | . ` ||  __||  __/  | | | | | | . ` ||  __|      |
|          \     `=./`,     '  | |\  || |___| |     | | | |_| | |\  || |___      |
|  +    .   '=.__.=' `='   *   \_| \_/\____/\_|     \_/  \___/\_| \_/\____/      |
|                                                              BY: @G_Sansigolo  |
+-------------------------------------------------------------------------------*/
#ifndef model
#define model
#define MAX_DIALOGS        (500)

typedef void    (*dialog_callback)(void);

static dialog_callback      dialogs[MAX_DIALOGS];

void draw_intro(void){
    define_dialogs();
    jo_clear_screen();
    set_background("ALERT.TGA", "BG");
    jo_set_printf_color_index(JO_COLOR_INDEX_White);
    jo_spin_wait(0);
    jo_clear_screen();
    draw_menu();
}

void draw_menu(void){
    jo_clear_screen();
    set_background("TITLE.TGA", "BG");
    jo_set_printf_color_index(JO_COLOR_INDEX_White);
    jo_printf(11, 23, "PRESS START BUTTON");
    jo_printf(25, 28, "@NeptuneEngine");
    jo_printf(1, 28, "@G_Sansigolo");
};

void start(void){
    jo_clear_screen();
    set_background("LETTER.TGA", "BG");
    dialog_000();
}

void dialog_000(void){
  d_n("You are Renne Bras, an ordinary",1);
    d("detective. You entered the business",2);
    d("seven years ago, just after the",3);
}

void dialog_001(void){
  d_n("end of your longest relationship.",1);
    d("You became famous for solving the",2);
    d("case of kidnappings in the capital.",3);
}

void dialog_002(void){
  d_n("Ever since you never picked up",1);
    d("jobs that involved disappearance ",2);
    d("cases.",3);
}

void dialog_003(void){
  d_n("Everything changed when days ago",1);
    d("you received a message from an old",2);
    d("friend.",3);
}

void dialog_004(void){
  d_n("Renne long time no see, I am sending",1);
    d("this message because I am running ",2);
    d("out of hope. A couple of my friends",3);
}

void dialog_005(void){
  d_n("went missing a week ago. I called",1);
    d("the police but they were unable to",2);
    d("find evidence to proceed.",3);
}

void dialog_006(void){
  d_n("I would like to hire you to investi-",1);
    d("gate this case. I live in Sao Bento",2);
    d("do Oeste, Minas Gerais, near the",3);
}

void dialog_007(void){
  d_n("border with Sao Paulo. Please help",1);
    d("me Renne, you are my only hope.",2);
    d("With love, - Julia.",3);
}

void dialog_008(void){
    jo_clear_screen();
    set_background("EXIT0.TGA", "BG");
    dialog_009();
}

void dialog_009(void){

  d_n("Sao Bento do Oeste",1);
    d("Minas Gerais - Brazil",2);
    d("City entrance",3);

}

void dialog_010(void){
  d_n("Meet me near the city entrence at 4PM",1);
    d("                             - Julia.",2);
    d("",3);
}

void dialog_011(void){
    jo_clear_screen();
    set_background("EXIT.TGA", "BG");
    dialog_012();
}

void dialog_012(void){
    h("[ Renne ]");
                d("Finally Sao Bento do Oeste",1);
    d("",2);
    d("",3);
}

void dialog_013(void){
    h("[ Julia ]");
                d("Rennee!",1);
    d("",2);
    d("",3);
}

void dialog_014(void){
    jo_clear_screen();
    set_background("EXIT2.TGA", "BG");
    dialog_015();
}

void dialog_015(void){
    h("[ Julia ]");
                d("You came!",1);
    d("",2);
    d("",3);
}

void dialog_016(void){
    h("[ Renne ]");
                d("I couldn't ignore an old",1);
    d("friend.",2);
    d("",3);
}

void dialog_017(void){
    h("[ Julia ]");
                d("Long time no see.",1);
    d("",2);
    d("",3);
}

void dialog_018(void){
    h("[ Renne ]");
                d("Yes, it does.",1);
    d("",2);
    d("",3);
}

void dialog_019(void){
    h("[ Renne ]");
                d("I would love to spend an",1);
    d("afternoon talking with you...",2);
    d("",3);
}

void dialog_020(void){
    h("[ Julia ]");
                d("Oh Right, the case.",1);
    d("",2);
    d("",3);
}

void dialog_021(void){
    h("[ Renne ]");
                d("I will ask you a few ",1);
    d("questions, feel free to ask back",2);
    d("anytime.",3);
}

void dialog_022(void){
    h("[ Julia ]");
                d("Okay.",1);
    d("",2);
    d("",3);
}

void dialog_023(void){
    h("[ Renne ]");
                d("Who are the couple of",1);
    d("friends you mentioned?",2);
    d("",3);
}

void dialog_024(void){
    h("[ Julia ]");
                d("Their names are Cristiano",1);
    d("Machado and Alice Zena. I known them",2);
    d("for almost three years.",3);
}

void dialog_025(void){
    h("[ Julia ]");
                d("Are they married, friends ",1);
    d("or something else?",2);
    d("",3);
}

void dialog_026(void){
    h("[ Julia ]");
                d("They are dating. I",1);
    d("remember Alice telling me that she",2);
    d("was waiting for Cris to ask her..",3);
}

void dialog_027(void){
    h("[ Renne ]");
                d("I see. What was their ",1);
    d("jobs?",2);
    d("",3);
}

void dialog_028(void){
    h("[ Julia ]");
                d("Cristiano was a lawyer,",1);
    d("he had an office on 15th March Street",2);
    d("the main street of the city.",3);
}

void dialog_029(void){
    h("[ Julia ]");
                d("Alice was a Portuguese ",1);
    d("teacher at the school I work at.",2);
    d("",3);
}

void dialog_030(void){
    h("[ Renne ]");
                d("How would you describe",1);
    d("their relationship?",2);
    d("",3);
}

void dialog_031(void){
    h("[ Julia ]");
                d("They seemed very close, ",1);
    d("a perfect couple. I never saw them ",2);
    d("fight...",3);
}

void dialog_032(void){
    h("[ Renne ]");
                d("Hum.. ",1);
    d("",2);
    d("",3);
}

void dialog_033(void){
    h("[ Julia ]");
                d("I'm sorry did I say",1);
    d("something?",2);
    d("",3);
}

void dialog_034(void){
    h("[ Renne ]");
                d("No, I just remembered one",1);
    d("thing... Please continue.",2);
    d("",3);
}

void dialog_035(void){
    h("[ Julia ]");
                d("They were... I mean they",1);
    d("are a beautiful couple.",2);
    d("",3);
}

void dialog_036(void){
    h("[ Renne ]");
                d("As I understand it, you",1);
    d("were much closer to Alice than to",2);
    d("Cristiano right?",3);
}

void dialog_037(void){
    h("[ Julia ]");
                d("Yes.",1);
    d("",2);
    d("",3);
}

void dialog_038(void){
    h("[ Renne ]");
                d("About Cristiano, you knew ",1);
    d("something about his life beyond what ",2);
    d("Alice told you...",3);
}

void dialog_039(void){
    h("[ Julia ]");
                d("As far as I remember he",1);
    d("used to go to drink in the bar in ",2);
    d("front of his office.",3);
}

void dialog_040(void){
    h("[ Renne ]");
                d("Did you knew Cristiano",1);
    d("before his relationship with Alice?",2);
    d("",3);
}

void dialog_041(void){
    h("[ Julia ]");
                d("Just by sight.",1);
    d("",2);
    d("",3);
}

void dialog_042(void){
    h("[ Renne ]");
                d("Right. About the",1);
    d("disappearance, how you noticed that",2);
    d("both were gone.",3);
}

void dialog_043(void){
    h("[ Julia ]");
                d("They don't have a car, so",1);
    d("I saw them every day walking to work.",2);
    d("",3);
}

void dialog_044(void){
    h("[ Julia ]");
                d("A week ago Alice said that ",1);
    d("she wanted to go to the lighthouse.",2);
    d("",3);
}

void dialog_045(void){
    h("[ Renne ]");
                d("Lighthouse?",1);
    d("",2);
    d("",3);
}

void dialog_046(void){
    h("[ Julia ]");
                d("Yes, the City lighthouse.",1);
    d("",2);
    d("",3);
}

void dialog_047(void){
    h("[ Renne ]");
                d("Why does this city have",1);
    d("a lighthouse? We are in Minas Gerais,",2);
    d("there no sea.",3);
}

void dialog_048(void){
    h("[ Julia ]");
                d("It is the city monument,",1);
    d("they built it to be a tourist spot ",2);
    d("for the city.",3);
}

void dialog_049(void){
    h("[ Renne ]");
                d("Anyway, let's continue.",1);
    d("",2);
    d("",3);
}

void dialog_050(void){
    h("[ Julia ]");
                d("Alice said she would go",1);
    d("to the lighthouse. Once I heard it, I",2);
    d("thought that Cristiano would ask her.",3);
}

void dialog_051(void){
    h("[ Renne ]");
                d("Oh.",1);
    d("",2);
    d("",3);
}

void dialog_052(void){
    h("[ Julia ]");
                d("Since then I have not ",1);
    d("heard any more news from the two, ",2);
    d("both have stopped showing up at their",3);
}

void dialog_053(void){
    h("[ Julia ]");
                d("work, the house has been ",1);
    d("abandoned, and from what I talked,",2);
    d("nobody has seen them since ever since",3);
}


void dialog_054(void){
    h("[ Renne ]");
                d("Copy, Is there anything ",1);
    d("else you want to tell me? ",2);
    d("",3);
}

void dialog_055(void){
    h("[ Julia ]");
                d("I don't think so.",1);
    d("",2);
    d("",3);
}

void dialog_056(void){
    h("[ Renne ]");
                d("Okay, I'm going to ",1);
    d("walk around town, to collect some",2);
    d("testimonies, if that's okay with you?",3);
}

void dialog_057(void){
    h("[ Julia ]");
                d("It's all right.",1);
    d("",2);
    d("",3);
}

void dialog_058(void){
    h("[ Julia ]");
                d("Do you want me to partner",1);
    d("you?",2);
    d("",3);
}

void dialog_059(void){
    h("[ Renne ]");
                d("I would love to, but it",1);
    d("can affect how people talk to me.",2);
    d("",3);
}

void dialog_060(void){
    h("[ Julia ]");
                d("I never thought this",1);
    d("way.",2);
    d("",3);
}

void dialog_061(void){
    h("[ Renne ]");
                d("It's not a big deal, ",1);
    d("it's just one of my working habits.",2);
    d("",3);
}

void dialog_062(void){
    h("[ Julia ]");
                d("Have a nice day then.",1);
    d("",2);
    d("",3);
}

void dialog_063(void){
    h("[ Renne ]");
                d("You too.",1);
    d("",2);
    d("",3);
}

void dialog_064(void){
    h("[ Renne ]");
                d("And before I forget, after ",1);
    d("all this we are going to have a real",2);
    d("dinner together.",3);
}

void dialog_065(void){
    h("[ Julia ]");
                d("Oh do you still remember",1);
    d("that...",2);
    d("",3);
}

void dialog_066(void){
    h("[ Julia ]");
                d("Ok, it is settled then.",1);
    d("",2);
    d("",3);
}

void dialog_067(void){
    jo_clear_screen();
    set_background("STREET0.TGA", "BG");
    dialog_068();
}

void dialog_068(void){
  d_n("Sao Bento do Oeste",1);
    d("15th March Street",2);
    d("Minutes later",3);
}

void dialog_069(void){
    jo_clear_screen();
    set_background("STREET.TGA", "BG");
    dialog_070();
}

void dialog_070(void){
    h("[ Sabrina ]");
                d(" Damn it! Who the hell is",1);
    d("doing this.",2);
    d("",3);
}

void dialog_071(void){
    h("[ Sabrina ]");
                d(" I will find out who is",1);
    d("responsible for these burned  ",2);
    d("dumpsters.",3);
}

void dialog_072(void){
    h("[ Sabrina ]");
                d(" Anyway, here I am ",1);
    d("Cristiano's office. I will put an ",2);
    d("end to this. ",3);
}

void dialog_073(void){
    h("[ Renne ]");
                d("Hey you, stop right there. ",1);
    d("",2);
    d("",3);
}

void dialog_074(void){
    jo_clear_screen();
    set_background("STREET2.TGA", "BG");
    dialog_075();
}

void dialog_075(void){
    h("[ Renne ]");
                d("You can't go in there! ",1);
    d("This place is close due to an ",2);
    d("investigation.",3);
}

void dialog_076(void){
    h("[ Sabrina ]");
                d(" I know that's why I'm ",1);
    d("here.",2);
    d("",3);
}

void dialog_077(void){
    h("[ Renne ]");
                d("Who are you?",1);
    d("",2);
    d("",3);
}

void dialog_078(void){
    h("[ Sabrina ]");
                d(" You first, it was you who  ",1);
    d("showed up accusing me.",2);
    d("",3);
}

void dialog_079(void){
    h("[ Renne ]");
                d("Okay, my name is Renne",1);
    d("Bras, I'm a hired detective. I'm",2);
    d("working on this case.",3);
}

void dialog_080(void){
    h("[ Sabrina ]");
                d(" Julia's friend?",1);
    d("",2);
    d("",3);
}

void dialog_081(void){
    h("[ Renne ]");
                d("Yes, and who are you?",1);
    d("",2);
    d("",3);
}

void dialog_082(void){
    h("[ Sabrina ]");
                d(" My name is Sabrina I am",1);
    d("a teacher at the same school as Alice",2);
    d("I am investigating this case solo.",3);
}

void dialog_083(void){
    h("[ Renne ]");
                d("I see. Let's go in  ",1);
    d("together then.",2);
    d("",3);
}

void dialog_084(void){
    h("[ Renne ]");
                d("But first, let me ask you ",1);
    d("a few questions.",2);
    d("",3);
}

void dialog_085(void){
    h("[ Renne ]");
                d("Could you think of anyone ",1);
    d("behind  all this.",2);
    d("",3);
}

void dialog_086(void){
    h("[ Sabrina ]");
                d(" Cristiano.",1);
    d("",2);
    d("",3);
}

void dialog_087(void){
    h("[ Renne ]");
                d("Oh..",1);
    d("",2);
    d("",3);
}

void dialog_088(void){
    h("[ Renne ]");
                d("Why him? ",1);
    d("",2);
    d("",3);
}

void dialog_089(void){
    h("[ Sabrina ]");
                d(" He always seemed to hide ",1);
    d("something. Alice said it was his way, ",2);
    d("but I always had my suspicions.",3);
}

void dialog_090(void){
    h("[ Renne ]");
                d("About Alice, do you think ",1);
    d("she was taken to the lighthouse by ",2);
    d("force?",3);
}

void dialog_091(void){
    h("[ Sabrina ]");
                d(" No, she liked it there. ",1);
    d("I have a strong suspicion that she ",2);
    d("went with him without knowing more... ",3);
}

void dialog_092(void){
    h("[ Renne ]");
                d("Do you think it's a murder",1);
    d("case?",2);
    d("",3);
}

void dialog_093(void){
    h("[ Sabrina ]");
                d(" Yes, after the police  ",1);
    d("found the car they used abandoned at",2);
    d("the ligthouse I felt it was a murder. ",3);
}

void dialog_094(void){
    h("[ Renne ]");
                d("Wait, Julia told me they",1);
    d("didn't have a car. ",2);
    d("",3);
}

void dialog_095(void){
    h("[ Sabrina ]");
                d(" They bought one recently. ",1);
    d("I know because I was at the car sale ",2);
    d("that day. ",3);
}

void dialog_096(void){
    h("[ Renne ]");
                d("Where and when this car",1);
    d("sale happen? And what car was it?",2);
    d("",3);
}

void dialog_097(void){
    h("[ Sabrina ]");
                d(" It was in the church ",1);
    d("square, about ten days ago. And it",2);
    d("was a green Golf.",3);
}

void dialog_098(void){
    h("[ Renne ]");
                d("Before you came here to ",1);
    d("investigate the office, you found ",2);
    d("anything else suspicious?",3);
}

void dialog_099(void){
    h("[ Sabrina ]");
                d(" Let me see..",1);
    d("",2);
    d("",3);
}

void dialog_100(void){
    h("[ Sabrina ]");
                d(" The burned dumpsters.",1);
    d("",2);
    d("",3);
}

void dialog_101(void){
    h("[ Sabrina ]");
                d(" Starting a week ago, ",1);
    d("someone begin to set fire to the ",2);
    d("city's dumpsters.",3);
}

void dialog_102(void){
    h("[ Renne ]");
                d("That time interval, there",1);
    d("is something there.",2);
    d("",3);
}

void dialog_103(void){
    h("[ Sabrina ]");
                d(" Anything else? ",1);
    d(" ",2);
    d("",3);
}

void dialog_104(void){
  d_n("Choose your next question",1);
    d("  Press B to ask about Alice",2);
    d("  Press C to ask about Julia",3);
}

void dialog_401(void){
    h("[ Renne ]");
                d("Tell me more about Alice",1);
    d("how was she, what she liked to do in",2);
    d("her spare time.. anything.",3);
}

void dialog_402(void){
    h("[ Sabrina ]");
                d(" Alice? She was lovely.",1);
    d("We often travel together, either ",2);
    d("to the waterfall or anywhere else.",3);
}

void dialog_403(void){
    h("[ Renne ]");
                d("You two were very close.. ",1);
    d("I imagine that after she fell in love",2);
    d("with Cristiano everything changed.",3);
}

void dialog_404(void){
    h("[ Sabrina ]");
                d(" Yes, I considered that ",1);
    d("to be the break of our friendship.",2);
    d("",3);
}

void dialog_405(void){
    h("[ Renne ]");
                d("Hmm. ",1);
    d("",2);
    d("",3);
}

void dialog_406(void){
    h("[ Renne ]");
                d("Despite everything you ",1);
    d("still have a feeling for Alice, ",2);
    d("that's why you want this case.",3);
}

void dialog_407(void){
    h("[ Sabrina ]");
                d(" ... ",1);
    d("",2);
    d("",3);
}

void dialog_408(void){
    h("[ Renne ]");
                d("Without further ado, let's",1);
    d("enter Cristiano's office. ",2);
    d("",3);
}

void dialog_411(void){
    h("[ Renne ]");
                d("Tell me more about Julia",1);
    d("how was she, what she liked to do in",2);
    d("her spare time.. anything.",3);
}

void dialog_412(void){
    h("[ Sabrina ]");
                d(" Julia? She is a good ",1);
    d("friend, we have worked together for ",2);
    d("years.",3);
}

void dialog_413(void){
    h("[ Renne ]");
                d("What about her social ",1);
    d("life?",2);
    d("",3);
}

void dialog_414(void){
    h("[ Sabrina ]");
                d(" She is very closed, ",1);
    d("since I met her I have never seen her",2);
    d("with anyone for more than a month.",3);
}

void dialog_415(void){
    h("[ Renne ]");
                d("The same old Julia. ",1);
    d("",2);
    d("",3);
}

void dialog_416(void){
    h("[ Renne ]");
                d("Do you believe that ",1);
    d("Julia would have hidden anything ",2);
    d("from my investigation?",3);
}

void dialog_417(void){
    h("[ Sabrina ]");
                d(" Her social life. From ",1);
    d("what I know of Julia, I assume you ",2);
    d("two have a past. She probably hid",3);
}

void dialog_418(void){
    h("[ Sabrina ]");
                d(" information about her ",1);
    d("mental health. ",2);
    d("",3);
}

void dialog_419(void){
    h("[ Renne ]");
                d("Hmm. I can't do ",1);
    d("anything about this right now. ",2);
    d("",3);
}

void dialog_420(void){
    h("[ Renne ]");
                d("Without further ado, let's",1);
    d("enter Cristiano's office. ",2);
    d("",3);
}

void dialog_105(void){
    jo_clear_screen();
    set_background("STREET0.TGA", "BG");
    dialog_106();
}

void dialog_106(void){
  d_n("Thirty minutes later.",1);
    d(" ",2);
    d(" ",3);
}

void dialog_107(void){
  d_n("You and Sabrina spent thirty minutes ",1);
    d("searching Cristiano's office.",2);
    d(" ",3);
}

void dialog_108(void){
    jo_clear_screen();
    set_background("STREET2.TGA", "BG");
    dialog_106();
}

void dialog_109(void){
    h("[ Sabrina ]");
                d(" Damn it. ",1);
    d("",2);
    d("",3);
}

void dialog_110(void){
    h("[ Sabrina ]");
                d(" There's nothing here. ",1);
    d("",2);
    d("",3);
}

void dialog_111(void){
    h("[ Renne ]");
                d("This means that the ",1);
    d("reason behind the disappearance ",2);
    d("is not related work.",3);
}

void dialog_112(void){
    h("[ Sabrina ]");
                d(" Hmm. ",1);
    d("",2);
    d("",3);
}

void dialog_113(void){
    h("[ Sabrina ]");
                d(" That was a dead end.",1);
    d("",2);
    d("",3);
}

void dialog_114(void){
    h("[ Renne ]");
                d("... ",1);
    d("",2);
    d("",3);
}

void dialog_115(void){
    h("[ Renne ]");
                d("Not so much.",1);
    d("",2);
    d("",3);
}

void dialog_116(void){
    h("[ Sabrina ]");
                d(" Why?",1);
    d("",2);
    d("",3);
}

void dialog_117(void){
    h("[ Renne ]");
                d("Julia told me that ",1);
    d("Cristiano often came to drink in the ",2);
    d("front bar of his office.",3);
}

void dialog_118(void){
    h("[ Renne ]");
                d("We can work on the ",1);
    d("hypothesis that Cristiano made his ",2);
    d("decisions at the bar, not at work. ",3);
}

void dialog_119(void){
    h("[ Sabrina ]");
                d(" Makes sense.",1);
    d("",2);
    d("",3);
}

void dialog_120(void){
    h("[ Sabrina ]");
                d(" What will you do?",1);
    d("",2);
    d("",3);
}

void dialog_121(void){
    h("[ Renne ]");
                d("Continue to investigate..  ",1);
    d("You can continue to investigate on  ",2);
    d("your own if you want. ",3);
}

void dialog_122(void){
    h("[ Sabrina ]");
                d(" OK good luck.",1);
    d("",2);
    d("",3);
}

void dialog_123(void){
    h("[ Renne ]");
                d("Same to you.  ",1);
    d("",2);
    d("",3);
}

void dialog_124(void){
    jo_clear_screen();
    set_background("CHURCH.TGA", "BG");
    dialog_106();
}

void dialog_125(void){
  d_n("Sao Bento do Oeste",1);
    d("Church square",2);
    d("Minutes later",3);
}

void dialog_126(void){
    jo_clear_screen();
    set_background("CHURCH1.TGA", "BG");
    dialog_127();
}

void dialog_127(void){
    h("[ Carlos ]");
                d("My job here is done.",1);
    d(" ",2);
    d(" ",3);
}

void dialog_128(void){
    jo_clear_screen();
    set_background("CHURCH2.TGA", "BG");
    dialog_129();
}

void dialog_129(void){
    h("[ Renne ]");
                d("Hey... My name is Renne",1);
    d("I am running an investigation on a",2);
    d("disappearance in this city.",3);
}

void dialog_130(void){
    h("[ Renne ]");
                d("Can I talk to you for a ",1);
    d("second?",2);
    d("",3);
}

void dialog_131(void){
    h("[ Carlos ]");
                d("Of course. Ask me ",1);
    d("anything. ",2);
    d(" ",3);
}

void dialog_132(void){
    h("[ Renne ]");
                d("What is your name, age  ",1);
    d("and occupation?",2);
    d("",3);
}

void dialog_133(void){
    h("[ Carlos ]");
                d("My name Carlos, I am 18 ",1);
    d("years old and I work as a delivery ",2);
    d("man in the city.",3);
}

void dialog_134(void){
    h("[ Renne ]");
                d("Are the names Cristiano ",1);
    d("Machado and Alice Zena familiar to ",2);
    d("you?",3);
}

void dialog_135(void){
    h("[ Carlos ]");
                d("I don't think so, the   ",1);
    d("only Cristiano I know is Tulio.. ",2);
    d("",3);
}

void dialog_136(void){
    h("[ Renne ]");
                d("I don't know if we're  ",1);
    d("talking about the same person.",2);
    d("",3);
}

void dialog_137(void){
    h("[ Carlos ]");
                d("He's a dentist, he has ",1);
    d("a clinic at the entrance to the ",2);
    d("city.",3);
}

void dialog_138(void){
    h("[ Renne ]");
                d("Unfortunately it's not ",1);
    d("the man I'm looking for.",2);
    d("",3);
}

void dialog_139(void){
    h("[ Carlos ]");
                d("Sorry.",1);
    d("",2);
    d("",3);
}

void dialog_140(void){
    h("[ Renne ]");
                d("There is something else",1);
    d("that you can help.",2);
    d("",3);
}

void dialog_141(void){
    h("[ Renne ]");
                d("In this square, about ",1);
    d("ten days ago a car sale took place.",2);
    d("",3);
}

void dialog_142(void){
    h("[ Carlos ]");
                d("Yes, the monthly car ",1);
    d("sale.",2);
    d("",3);
}

void dialog_143(void){
    h("[ Renne ]");
                d("Monthly!? ",1);
    d("",2);
    d("",3);
}

void dialog_144(void){
    h("[ Carlos ]");
                d("Yes, every 15th day people ",1);
    d("with businesses around this square ",2);
    d("organize it.",3);
}

void dialog_145(void){
    h("[ Renne ]");
                d("Good... good...",1);
    d("",2);
    d("",3);
}

void dialog_146(void){
    h("[ Renne ]");
                d("At the last car sale a  ",1);
    d("man named Cristiano bought a green ",2);
    d("Golf.",3);
}

void dialog_147(void){
    h("[ Renne ]");
                d("I would like more   ",1);
    d("information about.",2);
    d("",3);
}

void dialog_148(void){
    h("[ Carlos ]");
                d("I remember that.",1);
    d("",2);
    d("",3);
}

void dialog_149(void){
    h("[ Renne ]");
                d("Tell me more about it.",1);
    d("",2);
    d("",3);
}

void dialog_150(void){
    h("[ Carlos ]");
                d("It was a disputed sale, ",1);
    d("the man who won... what was his ",2);
    d("name...",3);
}

void dialog_151(void){
    h("[ Renne ]");
                d("Cristiano.",1);
    d("",2);
    d("",3);
}

void dialog_152(void){
    h("[ Carlos ]");
                d("Yes. He was very ",1);
    d("interested in the vehicle.",2);
    d("",3);
}

void dialog_153(void){
    h("[ Renne ]");
                d("Do you remember if he  ",1);
    d("was with someone?",2);
    d("",3);
}

void dialog_154(void){
    h("[ Carlos ]");
                d("Hmm.",1);
    d("",2);
    d("",3);
}

void dialog_155(void){
    h("[ Carlos ]");
                d("He was, there was a  ",1);
    d("young man wearing a scarf with him.",2);
    d("",3);
}

void dialog_156(void){
    h("[ Renne ]");
                d("Wasn't there a woman with ",1);
    d("him?",2);
    d("",3);
}

void dialog_157(void){
    h("[ Carlos ]");
                d("No, it was just him and ",1);
    d("that man. ",2);
    d("",3);
}

void dialog_158(void){
    h("[ Renne ]");
                d("Do you think there was ",1);
    d("any bond between the two? Friends, ",2);
    d("co-workers, family ...",3);
}

void dialog_159(void){
    h("[ Carlos ]");
                d("I don't think so, they ",1);
    d("seemed distant. ",2);
    d("",3);
}

void dialog_160(void){
    h("[ Renne ]");
                d("I get it.",1);
    d("",2);
    d("",3);
}

void dialog_161(void){
    h("[ Carlos ]");
                d("Do you excuse me, I've ",1);
    d("a medical exam scheduled at the ",2);
    d("club today. I don't want to miss.",3);
}

void dialog_162(void){
    h("[ Renne ]");
                d("Okay, you have been of ",1);
    d("great help.",2);
    d("",3);
}

void dialog_163(void){
    h("[ Carlos ]");
                d("You're welcome.",1);
    d("",2);
    d("",3);
}

void dialog_164(void){
    jo_clear_screen();
    set_background("CHURCH.TGA", "BG");
    dialog_165();
}

void dialog_165(void){
  d_n("Thirty minutes later.",1);
    d(" ",2);
    d(" ",3);
}

void dialog_166(void){
  d_n("You approached a girl who was ",1);
    d("in the square. ",2);
    d(" ",3);
}

void dialog_167(void){
    jo_clear_screen();
    set_background("CHURCH3.TGA", "BG");
    dialog_168();
}

void dialog_168(void){
    h("[ Renne ]");
                d("My name is Renne, I am",1);
    d("running an investigation on the",2);
    d("disappearance in this city.",3);
}

void dialog_169(void){
    h("[ Renne ]");
                d("Can I ask you a few ",1);
    d("questions? It will take a few ",2);
    d("minutes.",3);
}

void dialog_170(void){
    h("[ Maria ]");
                d("Of course, take your time.",1);
    d("",2);
    d("",3);
}

void dialog_171(void){
    h("[ Renne ]");
                d("Perfect, first I would ",1);
    d("like to know your name, your age and ",2);
    d("your occupation.",3);
}

void dialog_172(void){
    h("[ Maria ]");
                d("My name is Maria Vieira, ",1);
    d("I'm 18 years old and I'm a student. I ",2);
    d("don't have a job.",3);
}

void dialog_173(void){
    h("[ Renne ]");
                d("Right... right...",1);
    d("",2);
    d("",3);
}

void dialog_174(void){
    h("[ Renne ]");
                d("Here in this square ",1);
    d("there is a car sale.",2);
    d("",3);
}

void dialog_175(void){
    h("[ Maria ]");
                d("The monthly car sale.",1);
    d(" ",2);
    d("",3);
}

void dialog_176(void){
    h("[ Renne ]");
                d("Precisely.",1);
    d(" ",2);
    d("",3);
}

void dialog_177(void){
    h("[ Renne ]");
                d("Were you at the last? I ",1);
    d("have some questions about her. ",2);
    d("",3);
}

void dialog_178(void){
    h("[ Maria ]");

                d("I didn't go to them, I",1);
    d("don't have the money to buy a car. ",2);
    d("",3);
}

void dialog_179(void){
    h("[ Renne ]");
                d("It makes sense, okay, I ",1);
    d("have another question.",2);
    d("",3);
}

void dialog_180(void){
    h("[ Renne ]");
                d("Are the names Cristiano ",1);
    d("Machado and Alice Zena familiar to ",2);
    d("you?",3);
}

void dialog_181(void){
    h("[ Maria ]");
                d("I know these two, they ",1);
    d("are famous in the city, Cristiano is ",2);
    d("the most famous lawyer in the city.",3);
}

void dialog_182(void){
    h("[ Maria ]");
                d("And Alice is a teacher, ",1);
    d("so all students know her, I think ",2);
    d("everyone knows these two.",3);
}

void dialog_183(void){
    h("[ Renne ]");
                d("I see. These two  ",1);
    d("disappeared a week ago in the city, I",2);
    d("am investigating their disappearance.",3);
}

void dialog_184(void){
    h("[ Maria ]");
                d("Oh my god... I didn't ",1);
    d("know that... I want to help you ",2);
    d("with whatever you needg.",3);
}

void dialog_185(void){
    h("[ Renne ]");
                d("Thanks, about that do  ",1);
    d("you remember any of the two doing ",2);
    d("something in the city? ",3);
}

void dialog_186(void){
    h("[ Maria ]");
                d("Let me see... I remember ",1);
    d("Alice going to work normally, the  ",2);
    d("last time I saw her.",3);
}

void dialog_187(void){
    h("[ Maria ]");
                d("About Cristiano.. I ",1);
    d("don't remember seeing him in ",2);
    d("the last few days.. ",3);
}

void dialog_188(void){
    h("[ Renne ]");
                d("About that... ",1);
    d("",2);
    d("",3);
}

void dialog_189(void){
    h("[ Maria ]");
                d("But just to make it ",1);
    d("clear, this is normal, he did ",2);
    d("not walk around the city much.",3);
}

void dialog_190(void){
    h("[ Maria ]");
                d("Anything else? ",1);
    d(" ",2);
    d("",3);
}

void dialog_191(void){
  d_n("Choose your next question",1);
    d("  Press B to ask about Sabrina",2);
    d("  Press C to ask about Carlos",3);
}

void dialog_421(void){
    h("[ Renne ]");
                d("Minutes ago I was",1);
    d("investigating with a teacher from ",2);
    d("the city, Sabrina Freire.",3);
}

void dialog_422(void){
    h("[ Maria ]");
                d("I know her.",1);
    d(" ",2);
    d("",3);
}

void dialog_423(void){
    h("[ Renne ]");
                d("Do you have information ",1);
    d("about her? So I can confirm her  ",2);
    d("testimony and build up a case.",3);
}

void dialog_424(void){
    h("[ Maria ]");
                d("Let me see, Sabrina is a",1);
    d("little stubborn, she is open-minded ",2);
    d("and understands everyone's problem. ",3);
}

void dialog_425(void){
    h("[ Maria ]");
                d("But once she puts something",1);
    d("in mind she doesn't give up.",2);
    d(" ",3);
}

void dialog_426(void){
    h("[ Renne ]");
                d("This is interesting.",1);
    d("",2);
    d("",3);
}

void dialog_427(void){
    h("[ Renne ]");
                d("Do you believe that ",1);
    d("Sabrina would have hidden anything ",2);
    d("from my investigation?",3);
}

void dialog_428(void){
    h("[ Maria ]");
                d("Absolutely not, she cares ",1);
    d("about Alice more than anyone.",2);
    d("",3);
}

void dialog_429(void){
    h("[ Renne ]");
                d("Why do you say that?",1);
    d("",2);
    d("",3);
}

void dialog_430(void){
    h("[ Maria ]");
                d("These two have been ",1);
    d("friends since childhood. The two ",2);
    d("grew up in this city.",3);
}

void dialog_431(void){
    h("[ Renne ]");
                d("I didn't know that, so it",1);
    d("really was the break of their",2);
    d("friendship.",3);
}

void dialog_432(void){
    h("[ Maria ]");
                d("It was something that",1);
    d("Sabrina told me. ",2);
    d("",3);
}

void dialog_433(void){
    h("[ Renne ]");
                d("About her not hiding, do ",1);
    d("you really think she wouldn't lie?",2);
    d("",3);
}

void dialog_434(void){
    h("[ Maria ]");
                d("Yes I think, she's  like  ",1);
    d("an open book. She always spoke during  ",2);
    d("classes about her life.",3);
}

void dialog_435(void){
    h("[ Renne ]");
                d("I see. ",1);
    d(" ",2);
    d("",3);
}

void dialog_441(void){
    h("[ Renne ]");
                d("Minutes ago, here in the",1);
    d("square I interrogated a boy named ",2);
    d("Carlos...",3);
}

void dialog_442(void){
    h("[ Maria ]");
                d("What about him?",1);
    d(" ",2);
    d("",3);
}

void dialog_443(void){
    h("[ Renne ]");
                d("Do you have information ",1);
    d("about him? So I can confirm his  ",2);
    d("testimony and build up a case.",3);
}

void dialog_444(void){
    h("[ Maria ]");
                d("Unfortunately I don't",1);
    d("know this Carlos you mentioned, when ",2);
    d("I arrived you were writing alone.",3);
}

void dialog_445(void){
    h("[ Renne ]");
                d("It's all right... ",1);
    d("",2);
    d("",3);
}

void dialog_446(void){
    h("[ Renne ]");
                d("And what about you? In ",1);
    d("the past ten days have you seen ",2);
    d("anything strange?",3);
}

void dialog_447(void){
    h("[ Maria ]");
                d("Let me see...",1);
    d(" ",2);
    d("",3);
}

void dialog_448(void){
    h("[ Maria ]");
                d("I remembered something... ",1);
    d("a week ago I saw two men fighting on ",2);
    d("the street.",3);
}

void dialog_449(void){
    h("[ Maria ]");
                d("They were shouting at ",1);
    d("each other in the middle of the  ",2);
    d("night.",3);
}

void dialog_450(void){
    h("[ Renne ]");
                d("This is probably a dead ",1);
    d("end...  ",2);
    d("",3);
}

void dialog_451(void){
    h("[ Renne ]");
                d("Do you remember ",1);
    d("something that was shouted? ",2);
    d("",3);
}

void dialog_452(void){
    h("[ Maria ]");
                d("I remember them saying ",1);
    d("something about dealing with loose",2);
    d("ends.",3);
}

void dialog_453(void){
    h("[ Renne ]");
                d("Interesting. ",1);
    d(" ",2);
    d("",3);
}

void dialog_192(void){
    h("[ Renne ]");
                d("Maria if you're not busy ",1);
    d("I have a few more questions for you. ",2);
    d("",3);
}

void dialog_193(void){
    h("[ Maria ]");
                d("Don't worry. I am not.",1);
    d("",2);
    d("",3);
}

void dialog_194(void){
    h("[ Renne ]");
                d("About Cristiano, you said ",1);
    d("he was popular in the city, why do  ",2);
    d("you say that?",3);
}

void dialog_195(void){
    h("[ Maria ]");
                d("He being a famous lawyer",1);
    d("in town, he had a lot of clients, so ",2);
    d("there was always someone who know him.",3);
}

void dialog_196(void){
    h("[ Renne ]");
                d("So he was good at what he",1);
    d("did.  ",2);
    d("",3);
}

void dialog_197(void){
    h("[ Maria ]");
                d("Yes, according to his",1);
    d("customers...",2);
    d("",3);
}

void dialog_198(void){
    h("[ Renne ]");
                d("Do you know anything ",1);
    d("about his work in the past ten days?",2);
    d("",3);
}

void dialog_199(void){
    h("[ Maria ]");
                d("No I'm sorry.",1);
    d("",2);
    d("",3);
}

void dialog_200(void){
    h("[ Renne ]");
                d("It's all right.",1);
    d("",2);
    d("",3);
}

void dialog_201(void){
    h("[ Renne ]");
                d("I had other questions ",1);
    d("about him but I think that as you ",2);
    d("don't know, I don't add much.",3);
}

void dialog_202(void){
    h("[ Maria ]");
                d("...",1);
    d("",2);
    d("",3);
}

void dialog_203(void){
    h("[ Renne ]");
                d("Before I forget, I wanted",1);
    d("to go to the city lighthouse, which",2);
    d("way is it?",3);
}

void dialog_204(void){
    h("[ Maria ]");
                d("Keeps going towards the ",1);
    d("border of Sao Paulo, and them turn ",2);
    d("left. It is impossible to get lost.",3);
}

void dialog_205(void){
    h("[ Renne ]");
                d("Got it. Thanks for ",1);
    d("everything Maria.",2);
    d("",3);
}

void dialog_206(void){
    h("[ Maria ]");
                d("It was nothing.",1);
    d("",2);
    d("",3);
}

void dialog_207(void){
    h("[ Renne ]");
                d("Is there anything I can ",1);
    d("do to make up for your lost time?",2);
    d("",3);
}

void dialog_208(void){
    h("[ Maria ]");
                d("Don't worry, I didn't ",1);
    d("have anything important to do. If ",2);
    d("only the club was open...",3);
}

void dialog_209(void){
    h("[ Maria ]");
                d("Anyway, have a nice day.",1);
    d("",2);
    d("",3);
}

void dialog_210(void){
    h("[ Renne ]");
                d("You too.",1);
    d("",2);
    d("",3);
}

void dialog_211(void){
    jo_clear_screen();
    set_background("LIGHT.TGA", "BG");
    dialog_212();
}

void dialog_212(void){
  d_n("Sao Bento do Oeste",1);
    d("City Lighthouse",2);
    d("Minutes later",3);
}

void dialog_213(void){
    jo_clear_screen();
    set_background("LIGHT1.TGA", "BG");
    dialog_214();
}

void dialog_214(void){
    h("[ Lucas ]");
                d("Finally.",1);
    d("",2);
    d("",3);
}

void dialog_215(void){
    h("[ Lucas ]");
                d("The city lighthouse.",1);
    d("",2);
    d("",3);
}

void dialog_216(void){
    h("[ Renne ]");
                d("Hey! You.",1);
    d("",2);
    d("",3);
}

void dialog_217(void){
    jo_clear_screen();
    set_background("LIGHT2.TGA", "BG");
    dialog_214();
}

void dialog_218(void){
    h("[ Renne ]");
                d("Stop right there.",1);
    d("",2);
    d("",3);
}

void dialog_219(void){
    h("[ Lucas ]");
                d("Okay..",1);
    d("",2);
    d("",3);
}

void dialog_220(void){
    h("[ Renne ]");
                d("Wait a second.",1);
    d("",2);
    d("",3);
}

void dialog_221(void){
    h("[ Lucas ]");
                d("It's all right.",1);
    d("",2);
    d("",3);
}

void dialog_222(void){
    h("[ Renne ]");
                d("You are a suspect in the ",1);
    d("disappearance of Cristiano Machado ",2);
    d("and Alice Zena.",3);
}

void dialog_223(void){
    h("[ Lucas ]");
                d("Me?!",1);
    d("",2);
    d("",3);
}

void dialog_224(void){
    h("[ Renne ]");
                d("Yes, I need your name, ",1);
    d("age and occupation. ",2);
    d("",3);
}

void dialog_225(void){
    h("[ Lucas ]");
                d("My name is Lucas Matos, I ",1);
    d("am 19 years old and I help out in the ",2);
    d("city hospital.",3);
}

void dialog_226(void){
    h("[ Renne ]");
                d("Where were you ten days ",1);
    d("ago? ",2);
    d("",3);
}

void dialog_227(void){
    h("[ Lucas ]");
                d("Ten days ago... it was a ",1);
    d("Saturday, so I was working at the ",2);
    d("hospital.",3);
}

void dialog_228(void){
    h("[ Renne ]");
                d("What can you tell me ",1);
    d("about the city's monthly car sale?",2);
    d("",3);
}

void dialog_229(void){
    h("[ Lucas ]");
                d("I don't know what you ",1);
    d("want me to answer, I don't know ",2);
    d("much about.",3);
}

void dialog_230(void){
    h("[ Lucas ]");
                d("I don't have the money to ",1);
    d("buy a car so I'm not going.",2);
    d("",3);
}

void dialog_231(void){
    h("[ Renne ]");
                d("Is the name Cristiano ",1);
    d("Machado similar to you?",2);
    d("",3);
}

void dialog_232(void){
    h("[ Lucas ]");
                d("Yes, he is the most famous",1);
    d("lawyer in town everyone knows him.",2);
    d("",3);
}

void dialog_233(void){
    h("[ Renne ]");
                d("I've information that a man",1);
    d("of your age, wearing a scarf, was with",2);
    d("Cristiano at this car sale.",3);
}

void dialog_234(void){
    h("[ Lucas ]");
                d("It wasn't me, I was not",1);
    d("close to Cristiano.",2);
    d("",3);
}

void dialog_235(void){
    h("[ Lucas ]");
                d("A young man close to ",1);
    d("Cristiano... it's probably his ",2);
    d("brother.",3);
}

void dialog_236(void){
    h("[ Renne ]");
                d("Does Cristiano have a",1);
    d("brother???",2);
    d("",3);
}

void dialog_237(void){
    h("[ Lucas ]");
                d("Yes, he has my age. ",1);
    d("",2);
    d("",3);
}

void dialog_238(void){
    h("[ Renne ]");
                d("Why Julia hide this ",1);
    d("information from me.",2);
    d("",3);
}

void dialog_239(void){
    h("[ Lucas ]");
                d("Few people know about his ",1);
    d("brother. ",2);
    d("",3);
}

void dialog_240(void){
    h("[ Lucas ]");
                d("It would be dangerous for ",1);
    d("a famous criminal lawyer to have a ",2);
    d("known relative in the city.",3);
}

void dialog_241(void){
    h("[ Renne ]");
                d("Uhm.",1);
    d("",2);
    d("",3);
}

void dialog_242(void){
    h("[ Renne ]");
                d("Makes sense.",1);
    d("",2);
    d("",3);
}

void dialog_243(void){
    h("[ Renne ]");
                d("Answer me some more ",1);
    d("questions...",2);
    d("",3);
}

void dialog_244(void){
    h("[ Lucas ]");
                d("Sure.",1);
    d("",2);
    d("",3);
}

void dialog_245(void){
    h("[ Renne ]");
                d("Here, the Lightouse, is ",1);
    d("the last place that Cristiano and ",2);
    d("Alice were seen, why you came here?",3);
}

void dialog_246(void){
    h("[ Lucas ]");
                d("I often come here to rest.",1);
    d("",2);
    d("",3);
}

void dialog_247(void){
    h("[ Renne ]");
                d("Got it.",1);
    d("",2);
    d("",3);
}

void dialog_248(void){
  d_n("Choose your next question",1);
    d("  Press B to ask about Maria",2);
    d("  Press C to ask about Cris's brother",3);
}


void dialog_461(void){
    h("[ Renne ]");
                d("Minutes ago, at the",1);
    d("square I interrogated a girl named ",2);
    d("Maria...",3);
}

void dialog_462(void){
    h("[ Lucas ]");
                d("Maria? Why did you bring ",1);
    d("that name?",2);
    d("",3);
}

void dialog_463(void){
    h("[ Renne ]");
                d("I just said... I ",1);
    d("interrogated her in the square ",2);
    d("minutes ago.",3);
}

void dialog_464(void){
    h("[ Lucas ]");
                d("Tss. ",1);
    d("",2);
    d("",3);
}

void dialog_465(void){
    h("[ Renne ]");
                d("Why are you so defensive?",1);
    d("",2);
    d("",3);
}

void dialog_466(void){
    h("[ Lucas ]");
                d("I know Maria Vieira, she  ",1);
    d("is my ex-girlfriend.",2);
    d("",3);
}

void dialog_467(void){
    h("[ Renne ]");
                d("I see, you are ",1);
    d("uncomfortable, you two must have had ",2);
    d("a bad break-up.",3);
}

void dialog_468(void){
    h("[ Lucas ]");
                d("It is not about that.",1);
    d("",2);
    d("",3);
}

void dialog_469(void){
    h("[ Lucas ]");
                d("I made a lot of mistakes ",1);
    d("in this life, especially in the way I ",2);
    d("treated Maria.",3);
}

void dialog_470(void){
    h("[ Renne ]");
                d("It is okay, I made ",1);
    d("mistakes in past relationships too.",2);
    d("",3);
}

void dialog_471(void){
    h("[ Renne ]");
                d("It is by recognizing our ",1);
    d("mistakes and learning from them that ",2);
    d("we can become better persons.",3);
}

void dialog_472(void){
    h("[ Lucas ]");
                d("Make sense, sorry about ",1);
    d("that, about before, what you asked",2);
    d("about  Maria?",3);
}

void dialog_473(void){
    h("[ Renne ]");
                d("Do you believe that ",1);
    d("Maria would have hidden anything ",2);
    d("from my investigation?",3);
}

void dialog_474(void){
    h("[ Lucas ]");
                d("Maria? Impossible, she ",1);
    d("always wants to help others, no ",2);
    d("matter the cost.",3);
}

void dialog_475(void){
    h("[ Lucas ]");
                d("This is good to know.",1);
    d("",2);
    d("",3);
}

void dialog_481(void){
    h("[ Renne ]");
                d("Tell me more about this ",1);
    d("brother of Cristiano, I want to know ",2);
    d("everything you know about him.",3);
}

void dialog_482(void){
    h("[ Lucas ]");
                d("I don't remember his ",1);
    d("first name, as far as I remember he ",2);
    d("was called Silva Macahado.",3);
}

void dialog_483(void){
    h("[ Renne ]");
                d("How was his relationship ",1);
    d("with his brother?",2);
    d("",3);
}

void dialog_484(void){
    h("[ Lucas ]");
                d("It was very complicated, ",1);
    d("they were always fighting, but not in ",2);
    d("public.",3);
}

void dialog_485(void){
    h("[ Renne ]");
                d("I see. What about his  ",1);
    d("body... was he strong... tall... ?",2);
    d("",3);
}

void dialog_486(void){
    h("[ Lucas ]");
                d("He was normal like me. I ",1);
    d("remember that he had black hair and a ",2);
    d("scar on his neck.",3);
}

void dialog_487(void){
    h("[ Renne ]");
                d("Copy. Do you have any ",1);
    d("other information about him that ",2);
    d("you want to tell me about?",3);
}

void dialog_488(void){
    h("[ Lucas ]");
                d("I believe that even if he",1);
    d("is not involved, Silva Macahdo will  ",2);
    d("know everything about what happened.",3);
}

void dialog_489(void){
    h("[ Renne ]");
                d("Why you say that?",1);
    d("",2);
    d("",3);
}

void dialog_490(void){
    h("[ Lucas ]");
                d("He was well informed, I",1);
    d("remember seeing Cristiano asking him",2);
    d("information about the city many times",3);
}

void dialog_491(void){
    h("[ Renne ]");
                d("Noted.",1);
    d("",2);
    d("",3);
}

void dialog_249(void){
    h("[ Renne ]");
                d("You were of great help ",1);
    d("Lucas. Thanks for everything.",2);
    d("",3);
}

void dialog_250(void){
    h("[ Lucas ]");
                d("It was nothing.",1);
    d("",2);
    d("",3);
}

void dialog_251(void){
    h("[ Renne ]");
                d("I will continue my ",1);
    d("investigation from here, have a ",2);
    d("great day.",3);
}

void dialog_252(void){
    h("[ Lucas ]");
                d("You too.",1);
    d("",2);
    d("",3);
}

void dialog_253(void){
    jo_clear_screen();
    set_background("LIGHT.TGA", "BG");
    dialog_254();
}

void dialog_254(void){
  d_n("Sao Bento do Oeste",1);
    d("City Lighthouse",2);
    d("Minutes later",3);
}

void dialog_255(void){
    jo_clear_screen();
    set_background("LIGHT3.TGA", "BG");
    dialog_256();
}

void dialog_256(void){
    d_n("You spent minutes looking for clues ",1);
    d("at the Lighthouse.",2);
    d("",3);
}

void dialog_257(void){
    d_n("Unfortunately you did not find ",1);
    d("anything useful.",2);
    d("",3);
}

void dialog_258(void){
    h("[ Renne ]");
                d("Damn it. Another dead end.",1);
    d("",2);
    d("",3);
}

void dialog_259(void){
    h("[ Renne ]");
                d("Let me rethink the case.",1);
    d("",2);
    d("",3);
}

void dialog_260(void){
    h("[ Renne ]");
                d("The two disappeared here, ",1);
    d("in the lighthouse, a week ago.",2);
    d("",3);
}

void dialog_261(void){
    h("[ Renne ]");
                d("They had a good relation.",1);
    d("However, Sabrina was suspicious of",2);
    d("Cristiano.",3);
}

void dialog_262(void){
    h("[ Renne ]");
                d("Let's think before, there ",1);
    d("was the car fair. Cristiano and his ",2);
    d("brother bought a green Golf.",3);
}

void dialog_263(void){
    h("[ Renne ]");
                d("Days later they arrived ",1);
    d("the Lighthouse by car. And they left ",2);
    d("the car here. What happened here?",3);
}

void dialog_264(void){
    h("[ Renne ]");
                d("Some pieces that still ",1);
    d("didn't fit were the discussion ",2);
    d("listened to by Maria.",3);
}

void dialog_265(void){
    h("[ Renne ]");
                d("..dealing with loose ends.",1);
    d("",2);
    d("",3);
}

void dialog_266(void){
    h("[ Renne ]");
                d("I can work with the ",1);
    d("theory that the two were Cristiano ",2);
    d("and his brother. ",3);
}

void dialog_267(void){
    h("[ Renne ]");
                d("Another piece that still ",1);
    d("doesn't fit is Cristiano's brother.",2);
    d("",3);
}

void dialog_268(void){
    h("[ Renne ]");
                d("If he knew everything, ",1);
    d("then he was able to maintain frequent ",2);
    d("contact without arousing suspicion.",3);
}

void dialog_269(void){
    h("[ Renne ]");
                d("To do so he would need ",1);
    d("to work with something that involved ",2);
    d("movement though the city.",3);
}

void dialog_270(void){
  d_n("Choose your next deduction",1);
    d("Press B - Carlos is Cristiano brother",2);
    d("Press C - Maria is Cristiano sister",3);
}

void dialog_501(void){
    h("[ Renne ]");
                d("Carlos is Cristiano's ",1);
    d("brother!",2);
    d("",3);
}

void dialog_502(void){
    h("[ Renne ]");
                d("It makes sense, once he ",1);
    d("knew that I was investigating his ",2);
    d("brother started to lie. ",3);
}

void dialog_503(void){
    h("[ Renne ]");
                d("This explains why he ",1);
    d("points to Lucas as a suspect, he made ",2);
    d("me chase a false culprit.",3);
}

void dialog_504(void){
    h("[ Renne ]");
                d("And the burned dumps are ",1);
    d("Carlos getting rid of thist brothers ",2);
    d("tracks.",3);
}

void dialog_505(void){
    h("[ Renne ]");
                d("Carlos has the answer for ",1);
    d("everything. I need to find him.",2);
    d("",3);
}

void dialog_506(void){
  d_n("You went to town to look for Carlos.",1);
    d("",2);
    d("",3);
}

void dialog_511(void){
    h("[ Renne ]");
                d("Maria is Cristiano's ",1);
    d("sister. It doesn't make a lot of ",2);
    d("sense.",3);
}

void dialog_512(void){
    d_n("You followed a wrong lead and came ",1);
    d("to a dead end.",2);
    d("",3);
}

void dialog_513(void){
    d_n("You failed to solve the case.",1);
    d("",2);
    d("",3);
}

void dialog_271(void){
    jo_clear_screen();
    set_background("EXIT0.TGA", "BG");
    dialog_272();
}

void dialog_272(void){

  d_n("Sao Bento do Oeste",1);
    d("Minas Gerais - Brazil",2);
    d("City entrance",3);
}

void dialog_273(void){
    jo_clear_screen();
    set_background("EXIT.TGA", "BG");
    dialog_274();
}

void dialog_274(void){
    h("[ Renne ]");
                d("Where is he?",1);
    d("",2);
    d("",3);
}

void dialog_275(void){
    h("[ Renne ]");
                d("I need to find him.",1);
    d("",2);
    d("",3);
}

void dialog_276(void){
    h("[ Renne ]");
                d("As soon as I find him, I",1);
    d("will have my answers.",2);
    d("",3);
}

void dialog_277(void){
    d_n("You spent minutes looking for Carlos ",1);
    d("at the entrance to the city.",2);
    d("",3);
}

void dialog_278(void){
    h("[ Renne ]");
                d("...",1);
    d("",2);
    d("",3);
}

void dialog_279(void){
    h("[ Renne ]");
                d("Damn it, he's not here.",1);
    d("",2);
    d("",3);
}

void dialog_280(void){
    d_n("You continued to walk around the city",1);
    d("to find him.",2);
    d("",3);
}

void dialog_281(void){
    jo_clear_screen();
    set_background("CHURCH.TGA", "BG");
    dialog_282();
}

void dialog_282(void){
  d_n("Thirty minutes later.",1);
    d(" ",2);
    d(" ",3);
}

void dialog_283(void){
    jo_clear_screen();
    set_background("CHURCH2.TGA", "BG");
    dialog_284();
}

void dialog_284(void){
    h("[ Renne ]");
                d("Hey, stop right there!",1);
    d("",2);
    d("",3);
}

void dialog_285(void){
    h("[ Carlos ]");
                d("...",1);
    d("",2);
    d("",3);
}

void dialog_286(void){
    h("[ Renne ]");
                d("You are under arrest for ",1);
    d("obstruction of evidence.",2);
    d("",3);
}

void dialog_287(void){
    jo_clear_screen();
    set_background("CHURCH.TGA", "BG");
    dialog_288();
}

void dialog_288(void){
  d_n("Thirty minutes later.",1);
    d(" ",2);
    d(" ",3);
}

void dialog_289(void){
    jo_clear_screen();
    set_background("CHURCH2.TGA", "BG");
    dialog_290();
}

void dialog_290(void){
    h("[ Renne ]");
                d("I will ask only once, what ",1);
    d("happened to Cristiano Machado and  ",2);
    d("Alice Zena.",3);
}

void dialog_291(void){
    h("[ Carlos ]");
                d("They are gone...",1);
    d("",2);
    d("",3);
}

void dialog_292(void){
    h("[ Renne ]");
                d("So it's really a murder ",1);
    d("case.",2);
    d("",3);
}

void dialog_293(void){
    h("[ Carlos ]");
                d("It wasn't supposed to end ",1);
    d("like this, Alice didn't do anything ",2);
    d("wrong.",3);
}

void dialog_294(void){
    h("[ Renne ]");
                d("What happened at the ",1);
    d("lighthouse? Answer me.",2);
    d("",3);
}

void dialog_295(void){
    h("[ Carlos ]");
                d("A break up!",1);
    d("",2);
    d("",3);
}

void dialog_296(void){
  d_n("You in your head started to put the",1);
    d("pieces together. ",2);
    d(" ",3);
}

void dialog_297(void){
    h("[ Renne ]");
                d("After something involving",1);
    d("the purchase of the car ten days ago.",2);
    d("Alice and Cristiano fought.",3);
}

void dialog_298(void){
    h("[ Renne ]");
                d("At the lighthouse, Alice ",1);
    d("announced the end of their ",2);
    d("relationship.",3);
}

void dialog_299(void){
    h("[ Renne ]");
                d("Cristiano was shocked.",1);
    d("",2);
    d("",3);
}

void dialog_300(void){
    h("[ Renne ]");
                d("You, as well informed, ",1);
    d("found out about the end of their ",2);
    d("relationship.",3);
}

void dialog_301(void){
  d_n("Choose who you think is the culprit",1);
    d("  Press B to choose Cristiano",2);
    d("  Press C to choose Carlos",3);
}

void dialog_302(void){
    h("[ Renne ]");
                d("Cristiano is behind all ",1);
    d("this. ",2);
    d("",3);
}

void dialog_531(void){
    h("[ Renne ]");
                d("You are behind all this.",1);
    d(" ",2);
    d("",3);
}

void dialog_532(void){
    d_n("You chose the wrong culprit. ",1);
    d("",2);
    d("",3);
}

void dialog_533(void){
    d_n("You failed to solve the case.",1);
    d("",2);
    d("",3);
}

void dialog_303(void){
    h("[ Carlos ]");
                d("...",1);
    d("",2);
    d("",3);
}

void dialog_304(void){
    h("[ Renne ]");
                d("Answer me.",1);
    d(" ",2);
    d("",3);
}

void dialog_305(void){
    h("[ Carlos ]");
                d("Yes. He was the one who ",1);
    d("made the whole mess.",2);
    d("",3);
}

void dialog_306(void){
    h("[ Renne ]");
                d("Explain everything to ",1);
    d("me, maybe I can do something to ",2);
    d("lower your sentence. ",3);
}

void dialog_307(void){
    h("[ Carlos ]");
                d("Cristiano liked to drink,",1);
    d("he always celebrated cases in the ",2);
    d("bar in front of his office.",3);
}

void dialog_308(void){
    h("[ Carlos ]");
                d("Ten days ago he managed ",1);
    d("to buy a car at the city car sale.",2);
    d(" A green Golf.",3);
}

void dialog_309(void){
    h("[ Carlos ]");
                d("Unfortunately, two days ",1);
    d("later, after one of his drinking, he ",2);
    d("did something horrible.",3);
}
void dialog_310(void){
    h("[ Carlos ]");
                d("He ran over a girl on ",1);
    d("the street.",2);
    d("",3);
}
void dialog_311(void){
    h("[ Carlos ]");
                d("To maintain his status ",1);
    d("as a renowned lawyer and hide his",2);
    d("crime he disappeared with the body.",3);
}

void dialog_312(void){
    h("[ Carlos ]");
                d("Until that moment, I ",1);
    d("didn't get involved. Everything ",2);
    d("happened at the lighthouse.",3);
}

void dialog_313(void){
    h("[ Carlos ]");
                d("I am not sure how it ",1);
    d("happened, but Alice found out about",2);
    d("it. ",3);
}

void dialog_314(void){
    h("[ Carlos ]");
                d("She break up with ",1);
    d("Cristiano, and said she would speak ",2);
    d("to the authorities. ",3);
}

void dialog_315(void){
    h("[ Carlos ]");
                d("There, Cristiano killed ",1);
    d("Alice.",2);
    d("",3);
}

void dialog_316(void){
    h("[ Carlos ]");
                d("A day later I found out ",1);
    d("about everything, and went down to ",2);
    d("clean my brother.",3);
}

void dialog_317(void){
    h("[ Carlos ]");
                d("It was a busy day.",1);
    d("",2);
    d("",3);
}

void dialog_318(void){
    h("[ Carlos ]");
                d("Unfortunately the loss",1);
    d("of his girlfriend made Cristiano ",2);
    d("lose the will to live.",3);
}

void dialog_319(void){
    h("[ Carlos ]");
                d("And one day later hie ",1);
    d("take his own life.",2);
    d("",3);
}

void dialog_320(void){
    h("[ Carlos ]");
                d("Since then I have been ",1);
    d("burning evidence that could point to ",2);
    d("Cristiano. To cover the crime.",3);
}

void dialog_321(void){
    h("[ Carlos ]");
                d("For a brief moment I ",1);
    d("thought that what I was doing was ",2);
    d("right, I was helping my brother.",3);
}

void dialog_322(void){
    h("[ Carlos ]");
                d("But I was wrong, he ",1);
    d("didn't deserve it.",2);
    d("",3);
}

void dialog_323(void){
    h("[ Renne ]");
                d("I'm speechless.",1);
    d(" ",2);
    d("",3);
}

void dialog_324(void){
    h("[ Renne ]");
                d("With your brother dead, ",1);
    d("I hope you pay for your crimes. ",2);
    d("",3);
}

void define_dialogs(void){
    dialogs[0] = dialog_000;
    dialogs[1] = dialog_001;
    dialogs[2] = dialog_002;
    dialogs[3] = dialog_003;
    dialogs[4] = dialog_004;
    dialogs[5] = dialog_005;
    dialogs[6] = dialog_006;
    dialogs[7] = dialog_007;
    dialogs[8] = dialog_008;
    dialogs[9] = dialog_009;
    dialogs[10] = dialog_010;
    dialogs[11] = dialog_011;
    dialogs[12] = dialog_012;
    dialogs[13] = dialog_013;
    dialogs[14] = dialog_014;
    dialogs[15] = dialog_015;
    dialogs[16] = dialog_016;
    dialogs[17] = dialog_017;
    dialogs[18] = dialog_018;
    dialogs[19] = dialog_019;
    dialogs[20] = dialog_020;
    dialogs[21] = dialog_021;
    dialogs[22] = dialog_022;
    dialogs[23] = dialog_023;
    dialogs[24] = dialog_024;
    dialogs[25] = dialog_025;
    dialogs[26] = dialog_026;
    dialogs[27] = dialog_027;
    dialogs[28] = dialog_028;
    dialogs[29] = dialog_029;
    dialogs[30] = dialog_030;
    dialogs[31] = dialog_031;
    dialogs[32] = dialog_032;
    dialogs[33] = dialog_033;
    dialogs[34] = dialog_034;
    dialogs[35] = dialog_035;
    dialogs[36] = dialog_036;
    dialogs[37] = dialog_037;
    dialogs[38] = dialog_038;
    dialogs[39] = dialog_039;
    dialogs[40] = dialog_040;
    dialogs[41] = dialog_041;
    dialogs[42] = dialog_042;
    dialogs[43] = dialog_043;
    dialogs[44] = dialog_044;
    dialogs[45] = dialog_045;
    dialogs[46] = dialog_046;
    dialogs[47] = dialog_047;
    dialogs[48] = dialog_048;
    dialogs[49] = dialog_049;
    dialogs[50] = dialog_050;
    dialogs[51] = dialog_051;
    dialogs[52] = dialog_052;
    dialogs[53] = dialog_053;
    dialogs[54] = dialog_054;
    dialogs[55] = dialog_055;
    dialogs[56] = dialog_056;
    dialogs[57] = dialog_057;
    dialogs[58] = dialog_058;
    dialogs[59] = dialog_059;
    dialogs[60] = dialog_060;
    dialogs[61] = dialog_061;
    dialogs[62] = dialog_062;
    dialogs[63] = dialog_063;
    dialogs[64] = dialog_064;
    dialogs[65] = dialog_065;
    dialogs[66] = dialog_066;
    dialogs[67] = dialog_067;
    dialogs[68] = dialog_068;
    dialogs[69] = dialog_069;
    dialogs[70] = dialog_060;
    dialogs[71] = dialog_071;
    dialogs[72] = dialog_072;
    dialogs[73] = dialog_073;
    dialogs[74] = dialog_074;
    dialogs[75] = dialog_075;
    dialogs[76] = dialog_076;
    dialogs[77] = dialog_077;
    dialogs[78] = dialog_078;
    dialogs[79] = dialog_079;
    dialogs[80] = dialog_080;
    dialogs[81] = dialog_081;
    dialogs[82] = dialog_082;
    dialogs[83] = dialog_083;
    dialogs[84] = dialog_084;
    dialogs[85] = dialog_085;
    dialogs[86] = dialog_086;
    dialogs[87] = dialog_087;
    dialogs[88] = dialog_088;
    dialogs[89] = dialog_089;
    dialogs[90] = dialog_090;
    dialogs[91] = dialog_091;
    dialogs[92] = dialog_092;
    dialogs[93] = dialog_093;
    dialogs[94] = dialog_094;
    dialogs[95] = dialog_095;
    dialogs[96] = dialog_096;
    dialogs[97] = dialog_097;
    dialogs[98] = dialog_098;
    dialogs[99] = dialog_099;
    dialogs[100] = dialog_100;
    dialogs[101] = dialog_101;
    dialogs[102] = dialog_102;
    dialogs[103] = dialog_103;
    dialogs[104] = dialog_104;
    dialogs[105] = dialog_105;
    dialogs[106] = dialog_106;
    dialogs[107] = dialog_107;
    dialogs[108] = dialog_108;
    dialogs[109] = dialog_109;
    dialogs[110] = dialog_110;
    dialogs[111] = dialog_111;
    dialogs[112] = dialog_112;
    dialogs[113] = dialog_113;
    dialogs[114] = dialog_114;
    dialogs[115] = dialog_115;
    dialogs[116] = dialog_116;
    dialogs[117] = dialog_117;
    dialogs[118] = dialog_118;
    dialogs[119] = dialog_119;
    dialogs[120] = dialog_120;
    dialogs[121] = dialog_121;
    dialogs[122] = dialog_122;
    dialogs[123] = dialog_123;
    dialogs[124] = dialog_124;
    dialogs[125] = dialog_125;
    dialogs[126] = dialog_126;
    dialogs[127] = dialog_127;
    dialogs[128] = dialog_128;
    dialogs[129] = dialog_129;
    dialogs[130] = dialog_130;
    dialogs[131] = dialog_131;
    dialogs[132] = dialog_132;
    dialogs[133] = dialog_133;
    dialogs[134] = dialog_134;
    dialogs[135] = dialog_135;
    dialogs[136] = dialog_136;
    dialogs[137] = dialog_137;
    dialogs[138] = dialog_138;
    dialogs[139] = dialog_139;
    dialogs[140] = dialog_140;
    dialogs[141] = dialog_141;
    dialogs[142] = dialog_142;
    dialogs[143] = dialog_143;
    dialogs[144] = dialog_144;
    dialogs[145] = dialog_145;
    dialogs[146] = dialog_146;
    dialogs[147] = dialog_147;
    dialogs[148] = dialog_148;
    dialogs[149] = dialog_149;
    dialogs[150] = dialog_150;
    dialogs[151] = dialog_151;
    dialogs[152] = dialog_152;
    dialogs[153] = dialog_153;
    dialogs[154] = dialog_154;
    dialogs[155] = dialog_155;
    dialogs[156] = dialog_156;
    dialogs[157] = dialog_157;
    dialogs[158] = dialog_158;
    dialogs[159] = dialog_159;
    dialogs[160] = dialog_160;
    dialogs[161] = dialog_161;
    dialogs[162] = dialog_162;
    dialogs[163] = dialog_163;
    dialogs[164] = dialog_164;
    dialogs[165] = dialog_165;
    dialogs[166] = dialog_166;
    dialogs[167] = dialog_167;
    dialogs[168] = dialog_168;
    dialogs[169] = dialog_169;
    dialogs[170] = dialog_160;
    dialogs[171] = dialog_171;
    dialogs[172] = dialog_172;
    dialogs[173] = dialog_173;
    dialogs[174] = dialog_174;
    dialogs[175] = dialog_175;
    dialogs[176] = dialog_176;
    dialogs[177] = dialog_177;
    dialogs[178] = dialog_178;
    dialogs[179] = dialog_179;
    dialogs[180] = dialog_180;
    dialogs[181] = dialog_181;
    dialogs[182] = dialog_182;
    dialogs[183] = dialog_183;
    dialogs[184] = dialog_184;
    dialogs[185] = dialog_185;
    dialogs[186] = dialog_186;
    dialogs[187] = dialog_187;
    dialogs[188] = dialog_188;
    dialogs[189] = dialog_189;
    dialogs[190] = dialog_190;
    dialogs[191] = dialog_191;
    dialogs[192] = dialog_192;
    dialogs[193] = dialog_193;
    dialogs[194] = dialog_194;
    dialogs[195] = dialog_195;
    dialogs[196] = dialog_196;
    dialogs[197] = dialog_197;
    dialogs[198] = dialog_198;
    dialogs[199] = dialog_199;
    dialogs[200] = dialog_200;
    dialogs[201] = dialog_201;
    dialogs[202] = dialog_202;
    dialogs[203] = dialog_203;
    dialogs[204] = dialog_204;
    dialogs[205] = dialog_205;
    dialogs[206] = dialog_206;
    dialogs[207] = dialog_207;
    dialogs[208] = dialog_208;
    dialogs[209] = dialog_209;
    dialogs[210] = dialog_210;
    dialogs[211] = dialog_211;
    dialogs[212] = dialog_212;
    dialogs[213] = dialog_213;
    dialogs[214] = dialog_214;
    dialogs[215] = dialog_215;
    dialogs[216] = dialog_216;
    dialogs[217] = dialog_217;
    dialogs[218] = dialog_218;
    dialogs[219] = dialog_219;
    dialogs[220] = dialog_220;
    dialogs[221] = dialog_221;
    dialogs[222] = dialog_222;
    dialogs[223] = dialog_223;
    dialogs[224] = dialog_224;
    dialogs[225] = dialog_225;
    dialogs[226] = dialog_226;
    dialogs[227] = dialog_227;
    dialogs[228] = dialog_228;
    dialogs[229] = dialog_229;
    dialogs[230] = dialog_230;
    dialogs[231] = dialog_231;
    dialogs[232] = dialog_232;
    dialogs[233] = dialog_233;
    dialogs[234] = dialog_234;
    dialogs[235] = dialog_235;
    dialogs[236] = dialog_236;
    dialogs[237] = dialog_237;
    dialogs[238] = dialog_238;
    dialogs[239] = dialog_239;
    dialogs[240] = dialog_240;
    dialogs[241] = dialog_241;
    dialogs[242] = dialog_242;
    dialogs[243] = dialog_243;
    dialogs[244] = dialog_244;
    dialogs[245] = dialog_245;
    dialogs[246] = dialog_246;
    dialogs[247] = dialog_247;
    dialogs[248] = dialog_248;
    dialogs[249] = dialog_249;
    dialogs[250] = dialog_250;
    dialogs[251] = dialog_251;
    dialogs[252] = dialog_252;
    dialogs[253] = dialog_253;
    dialogs[254] = dialog_254;
    dialogs[255] = dialog_255;
    dialogs[256] = dialog_256;
    dialogs[257] = dialog_257;
    dialogs[258] = dialog_258;
    dialogs[259] = dialog_259;
    dialogs[260] = dialog_260;
    dialogs[261] = dialog_261;
    dialogs[262] = dialog_262;
    dialogs[263] = dialog_263;
    dialogs[264] = dialog_264;
    dialogs[265] = dialog_265;
    dialogs[266] = dialog_266;
    dialogs[267] = dialog_267;
    dialogs[268] = dialog_268;
    dialogs[269] = dialog_269;
    dialogs[270] = dialog_270;
    dialogs[271] = dialog_271;
    dialogs[272] = dialog_272;
    dialogs[273] = dialog_273;
    dialogs[274] = dialog_274;
    dialogs[275] = dialog_275;
    dialogs[276] = dialog_276;
    dialogs[277] = dialog_277;
    dialogs[278] = dialog_278;
    dialogs[279] = dialog_279;
    dialogs[280] = dialog_280;
    dialogs[281] = dialog_281;
    dialogs[282] = dialog_282;
    dialogs[283] = dialog_283;
    dialogs[284] = dialog_284;
    dialogs[285] = dialog_285;
    dialogs[286] = dialog_286;
    dialogs[287] = dialog_287;
    dialogs[288] = dialog_288;
    dialogs[289] = dialog_289;
    dialogs[290] = dialog_290;
    dialogs[291] = dialog_291;
    dialogs[292] = dialog_292;
    dialogs[293] = dialog_293;
    dialogs[294] = dialog_294;
    dialogs[295] = dialog_295;
    dialogs[296] = dialog_296;
    dialogs[297] = dialog_297;
    dialogs[298] = dialog_298;
    dialogs[299] = dialog_299;
    dialogs[300] = dialog_300;
    dialogs[301] = dialog_301;
    dialogs[302] = dialog_302;
    dialogs[303] = dialog_303;
    dialogs[304] = dialog_304;
    dialogs[305] = dialog_305;
    dialogs[306] = dialog_306;
    dialogs[307] = dialog_307;
    dialogs[308] = dialog_308;
    dialogs[309] = dialog_309;
    dialogs[310] = dialog_310;
    dialogs[311] = dialog_311;
    dialogs[312] = dialog_312;
    dialogs[313] = dialog_313;
    dialogs[314] = dialog_314;
    dialogs[315] = dialog_315;
    dialogs[316] = dialog_316;
    dialogs[317] = dialog_317;
    dialogs[318] = dialog_318;
    dialogs[319] = dialog_319;
    dialogs[320] = dialog_320;
    dialogs[321] = dialog_321;
    dialogs[322] = dialog_322;
    dialogs[323] = dialog_323;
    dialogs[324] = dialog_324;

    dialogs[401] = dialog_401;
    dialogs[402] = dialog_402;
    dialogs[403] = dialog_403;
    dialogs[404] = dialog_404;
    dialogs[405] = dialog_405;
    dialogs[406] = dialog_406;
    dialogs[407] = dialog_407;
    dialogs[408] = dialog_408;

    dialogs[411] = dialog_411;
    dialogs[412] = dialog_412;
    dialogs[413] = dialog_413;
    dialogs[414] = dialog_414;
    dialogs[415] = dialog_415;
    dialogs[416] = dialog_416;
    dialogs[417] = dialog_417;
    dialogs[418] = dialog_418;
    dialogs[419] = dialog_419;
    dialogs[420] = dialog_420;

    dialogs[421] = dialog_421;
    dialogs[422] = dialog_422;
    dialogs[423] = dialog_423;
    dialogs[424] = dialog_424;
    dialogs[425] = dialog_425;
    dialogs[426] = dialog_426;
    dialogs[427] = dialog_427;
    dialogs[428] = dialog_428;
    dialogs[429] = dialog_429;
    dialogs[430] = dialog_430;
    dialogs[431] = dialog_431;
    dialogs[432] = dialog_432;
    dialogs[433] = dialog_433;
    dialogs[434] = dialog_434;
    dialogs[435] = dialog_435;

    dialogs[441] = dialog_441;
    dialogs[442] = dialog_442;
    dialogs[443] = dialog_443;
    dialogs[444] = dialog_444;
    dialogs[445] = dialog_445;
    dialogs[446] = dialog_446;
    dialogs[447] = dialog_447;
    dialogs[448] = dialog_448;
    dialogs[449] = dialog_449;
    dialogs[450] = dialog_450;
    dialogs[451] = dialog_451;
    dialogs[452] = dialog_452;
    dialogs[453] = dialog_453;

    dialogs[461] = dialog_461;
    dialogs[462] = dialog_462;
    dialogs[463] = dialog_463;
    dialogs[464] = dialog_464;
    dialogs[465] = dialog_465;
    dialogs[466] = dialog_466;
    dialogs[467] = dialog_467;
    dialogs[468] = dialog_468;
    dialogs[469] = dialog_469;
    dialogs[470] = dialog_470;
    dialogs[471] = dialog_471;
    dialogs[472] = dialog_472;
    dialogs[473] = dialog_473;
    dialogs[474] = dialog_474;
    dialogs[475] = dialog_475;

    dialogs[481] = dialog_481;
    dialogs[482] = dialog_482;
    dialogs[483] = dialog_483;
    dialogs[484] = dialog_484;
    dialogs[485] = dialog_485;
    dialogs[486] = dialog_486;
    dialogs[487] = dialog_487;
    dialogs[488] = dialog_488;
    dialogs[489] = dialog_489;
    dialogs[490] = dialog_290;
    dialogs[491] = dialog_291;

    dialogs[501] = dialog_501;
    dialogs[502] = dialog_502;
    dialogs[503] = dialog_503;
    dialogs[504] = dialog_504;
    dialogs[505] = dialog_505;
    dialogs[506] = dialog_506;

    dialogs[511] = dialog_511;
    dialogs[512] = dialog_512;
    dialogs[513] = dialog_513;

    dialogs[531] = dialog_531;
    dialogs[532] = dialog_532;
    dialogs[533] = dialog_533;

}

#endif
