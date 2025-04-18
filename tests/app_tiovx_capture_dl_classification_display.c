/*
 *
 * Copyright (c) 2024 Texas Instruments Incorporated
 *
 * All rights reserved not granted herein.
 *
 * Limited License.
 *
 * Texas Instruments Incorporated grants a world-wide, royalty-free, non-exclusive
 * license under copyrights and patents it now or hereafter owns or controls to make,
 * have made, use, import, offer to sell and sell ("Utilize") this software subject to the
 * terms herein.  With respect to the foregoing patent license, such license is granted
 * solely to the extent that any such patent is necessary to Utilize the software alone.
 * The patent license shall not apply to any combinations which include this software,
 * other than combinations with devices manufactured by or for TI ("TI Devices").
 * No hardware patent is licensed hereunder.
 *
 * Redistributions must preserve existing copyright notices and reproduce this license
 * (including the above copyright notice and the disclaimer and (if applicable) source
 * code license limitations below) in the documentation and/or other materials provided
 * with the distribution
 *
 * Redistribution and use in binary form, without modification, are permitted provided
 * that the following conditions are met:
 *
 * *       No reverse engineering, decompilation, or disassembly of this software is
 * permitted with respect to any software provided in binary form.
 *
 * *       any redistribution and use are licensed by TI for use only with TI Devices.
 *
 * *       Nothing shall obligate TI to provide you with source code for the software
 * licensed and provided to you in object code.
 *
 * If software source code is provided to you, modification and redistribution of the
 * source code are permitted provided that the following conditions are met:
 *
 * *       any redistribution and use of the source code, including any resulting derivative
 * works, are licensed by TI for use only with TI Devices.
 *
 * *       any redistribution and use of any object code compiled from the source code
 * and any resulting derivative works, are licensed by TI for use only with TI Devices.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of its suppliers
 *
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * DISCLAIMER.
 *
 * THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TI AND TI'S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Pipeline:
 *
 * Capture(IMX390 2MP) --> VISS --> LDC --> MSC0 --> MSC1 --> DL_PRE_PROC --> TIDL
 *                                             |                                  |
 *                                             |                                   --> DL_POST_PROC --> Display
 *                                             |                                  |
 *                                             ---------------------------------->
 */

#include <tiovx_modules.h>
#include <tiovx_utils.h>
#include <stdio.h>
#include <unistd.h>

#if defined(SOC_AM62A) && defined(TARGET_OS_QNX)
#include "qnx_display_module.h"
#endif

#define NUM_ITERATIONS   (300)

#define APP_BUFQ_DEPTH   (1)
#define APP_NUM_CH       (1)

#if !(defined(SOC_AM62A) && defined(TARGET_OS_QNX))
#define VISS_INPUT_WIDTH  1936
#define VISS_INPUT_HEIGHT 1100

#else
#define VISS_INPUT_WIDTH  1936
#define VISS_INPUT_HEIGHT 1096
#endif

#define VISS_OUTPUT_WIDTH  (VISS_INPUT_WIDTH)
#define VISS_OUTPUT_HEIGHT (VISS_INPUT_HEIGHT)

#define LDC_INPUT_WIDTH  VISS_OUTPUT_WIDTH
#define LDC_INPUT_HEIGHT VISS_OUTPUT_HEIGHT

#define LDC_OUTPUT_WIDTH  1920
#define LDC_OUTPUT_HEIGHT 1080

#define MSC_INPUT_WIDTH  LDC_OUTPUT_WIDTH
#define MSC_INPUT_HEIGHT LDC_OUTPUT_HEIGHT

#if !(defined(SOC_AM62A) && defined(TARGET_OS_QNX))
#define MSC_OUTPUT_WIDTH  MSC_INPUT_WIDTH/2
#define MSC_OUTPUT_HEIGHT MSC_INPUT_HEIGHT/2

#else
#define MSC_OUTPUT_WIDTH  MSC_INPUT_WIDTH
#define MSC_OUTPUT_HEIGHT MSC_INPUT_HEIGHT
#endif

#define LDC_TABLE_WIDTH     (1920)
#define LDC_TABLE_HEIGHT    (1080)
#define LDC_DS_FACTOR       (2)
#define LDC_BLOCK_WIDTH     (64)
#define LDC_BLOCK_HEIGHT    (32)
#define LDC_PIXEL_PAD       (1)

#define SENSOR_NAME "SENSOR_SONY_IMX390_UB953_D3"
#define DCC_VISS TIOVX_MODULES_IMAGING_PATH"/imx390/linear/dcc_viss.bin"
#define DCC_LDC TIOVX_MODULES_IMAGING_PATH"/imx390/linear/dcc_ldc.bin"

#if !(defined(SOC_AM62A) && defined(TARGET_OS_QNX))
#define POST_PROC_OUT_WIDTH (1280)
#define POST_PROC_OUT_HEIGHT (720)

#else
#define POST_PROC_OUT_WIDTH   (1920)
#define POST_PROC_OUT_HEIGHT  (1080)
#endif

#if !(defined(SOC_AM62A) && defined(TARGET_OS_QNX))
#define TIDL_IO_CONFIG_FILE_PATH "/opt/model_zoo/ONR-CL-6360-regNetx-200mf/artifacts/subgraph_0_tidl_io_1.bin"
#define TIDL_NETWORK_FILE_PATH "/opt/model_zoo/ONR-CL-6360-regNetx-200mf/artifacts/subgraph_0_tidl_net.bin"

#else 
#define TIDL_IO_CONFIG_FILE_PATH "/ti_fs/model_zoo/ONR-CL-6360-regNetx-200mf/artifacts/subgraph_0_tidl_io_1.bin"
#define TIDL_NETWORK_FILE_PATH "/ti_fs/model_zoo/ONR-CL-6360-regNetx-200mf/artifacts/subgraph_0_tidl_net.bin"
#endif

const char imgnet_labels1[TIVX_DL_POST_PROC_MAX_NUM_CLASSNAMES][TIVX_DL_POST_PROC_MAX_SIZE_CLASSNAME] =
{
  "background",
  "tench, Tinca tinca",
  "goldfish, Carassius auratus",
  "great white shark, white shark, man-eater, man-eating shark, Carcharodon carcharias",
  "tiger shark, Galeocerdo cuvieri",
  "hammerhead, hammerhead shark",
  "electric ray, crampfish, numbfish, torpedo",
  "stingray",
  "cock",
  "hen",
  "ostrich, Struthio camelus",
  "brambling, Fringilla montifringilla",
  "goldfinch, Carduelis carduelis",
  "house finch, linnet, Carpodacus mexicanus",
  "junco, snowbird",
  "indigo bunting, indigo finch, indigo bird, Passerina cyanea",
  "robin, American robin, Turdus migratorius",
  "bulbul",
  "jay",
  "magpie",
  "chickadee",
  "water ouzel, dipper",
  "kite",
  "bald eagle, American eagle, Haliaeetus leucocephalus",
  "vulture",
  "great grey owl, great gray owl, Strix nebulosa",
  "European fire salamander, Salamandra salamandra",
  "common newt, Triturus vulgaris",
  "eft",
  "spotted salamander, Ambystoma maculatum",
  "axolotl, mud puppy, Ambystoma mexicanum",
  "bullfrog, Rana catesbeiana",
  "tree frog, tree-frog",
  "tailed frog, bell toad, ribbed toad, tailed toad, Ascaphus trui",
  "loggerhead, loggerhead turtle, Caretta caretta",
  "leatherback turtle, leatherback, leathery turtle, Dermochelys coriacea",
  "mud turtle",
  "terrapin",
  "box turtle, box tortoise",
  "banded gecko",
  "common iguana, iguana, Iguana iguana",
  "American chameleon, anole, Anolis carolinensis",
  "whiptail, whiptail lizard",
  "agama",
  "frilled lizard, Chlamydosaurus kingi",
  "alligator lizard",
  "Gila monster, Heloderma suspectum",
  "green lizard, Lacerta viridis",
  "African chameleon, Chamaeleo chamaeleon",
  "Komodo dragon, Komodo lizard, dragon lizard, giant lizard, Varanus komodoensis",
  "African crocodile, Nile crocodile, Crocodylus niloticus",
  "American alligator, Alligator mississipiensis",
  "triceratops",
  "thunder snake, worm snake, Carphophis amoenus",
  "ringneck snake, ring-necked snake, ring snake",
  "hognose snake, puff adder, sand viper",
  "green snake, grass snake",
  "king snake, kingsnake",
  "garter snake, grass snake",
  "water snake",
  "vine snake",
  "night snake, Hypsiglena torquata",
  "boa constrictor, Constrictor constrictor",
  "rock python, rock snake, Python sebae",
  "Indian cobra, Naja naja",
  "green mamba",
  "sea snake",
  "horned viper, cerastes, sand viper, horned asp, Cerastes cornutus",
  "diamondback, diamondback rattlesnake, Crotalus adamanteus",
  "sidewinder, horned rattlesnake, Crotalus cerastes",
  "trilobite",
  "harvestman, daddy longlegs, Phalangium opilio",
  "scorpion",
  "black and gold garden spider, Argiope aurantia",
  "barn spider, Araneus cavaticus",
  "garden spider, Aranea diademata",
  "black widow, Latrodectus mactans",
  "tarantula",
  "wolf spider, hunting spider",
  "tick",
  "centipede",
  "black grouse",
  "ptarmigan",
  "ruffed grouse, partridge, Bonasa umbellus",
  "prairie chicken, prairie grouse, prairie fowl",
  "peacock",
  "quail",
  "partridge",
  "African grey, African gray, Psittacus erithacus",
  "macaw",
  "sulphur-crested cockatoo, Kakatoe galerita, Cacatua galerita",
  "lorikeet",
  "coucal",
  "bee eater",
  "hornbill",
  "hummingbird",
  "jacamar",
  "toucan",
  "drake",
  "red-breasted merganser, Mergus serrator",
  "goose",
  "black swan, Cygnus atratus",
  "tusker",
  "echidna, spiny anteater, anteater",
  "platypus, duckbill, duckbilled platypus, duck-billed platypus, Ornithorhynchus anatinus",
  "wallaby, brush kangaroo",
  "koala, koala bear, kangaroo bear, native bear, Phascolarctos cinereus",
  "wombat",
  "jellyfish",
  "sea anemone, anemone",
  "brain coral",
  "flatworm, platyhelminth",
  "nematode, nematode worm, roundworm",
  "conch",
  "snail",
  "slug",
  "sea slug, nudibranch",
  "chiton, coat-of-mail shell, sea cradle, polyplacophore",
  "chambered nautilus, pearly nautilus, nautilus",
  "Dungeness crab, Cancer magister",
  "rock crab, Cancer irroratus",
  "fiddler crab",
  "king crab, Alaska crab, Alaskan king crab, Alaska king crab, Paralithodes camtschatica",
  "American lobster, Northern lobster, Maine lobster, Homarus americanus",
  "spiny lobster, langouste, rock lobster, crawfish, crayfish, sea crawfish",
  "crayfish, crawfish, crawdad, crawdaddy",
  "hermit crab",
  "isopod",
  "white stork, Ciconia ciconia",
  "black stork, Ciconia nigra",
  "spoonbill",
  "flamingo",
  "little blue heron, Egretta caerulea",
  "American egret, great white heron, Egretta albus",
  "bittern",
  "crane",
  "limpkin, Aramus pictus",
  "European gallinule, Porphyrio porphyrio",
  "American coot, marsh hen, mud hen, water hen, Fulica americana",
  "bustard",
  "ruddy turnstone, Arenaria interpres",
  "red-backed sandpiper, dunlin, Erolia alpina",
  "redshank, Tringa totanus",
  "dowitcher",
  "oystercatcher, oyster catcher",
  "pelican",
  "king penguin, Aptenodytes patagonica",
  "albatross, mollymawk",
  "grey whale, gray whale, devilfish, Eschrichtius gibbosus, Eschrichtius robustus",
  "killer whale, killer, orca, grampus, sea wolf, Orcinus orca",
  "dugong, Dugong dugon",
  "sea lion",
  "Chihuahua",
  "Japanese spaniel",
  "Maltese dog, Maltese terrier, Maltese",
  "Pekinese, Pekingese, Peke",
  "Shih-Tzu",
  "Blenheim spaniel",
  "papillon",
  "toy terrier",
  "Rhodesian ridgeback",
  "Afghan hound, Afghan",
  "basset, basset hound",
  "beagle",
  "bloodhound, sleuthhound",
  "bluetick",
  "black-and-tan coonhound",
  "Walker hound, Walker foxhound",
  "English foxhound",
  "redbone",
  "borzoi, Russian wolfhound",
  "Irish wolfhound",
  "Italian greyhound",
  "whippet",
  "Ibizan hound, Ibizan Podenco",
  "Norwegian elkhound, elkhound",
  "otterhound, otter hound",
  "Saluki, gazelle hound",
  "Scottish deerhound, deerhound",
  "Weimaraner",
  "Staffordshire bullterrier, Staffordshire bull terrier",
  "American Staffordshire terrier, Staffordshire terrier, American pit bull terrier, pit bull terrier",
  "Bedlington terrier",
  "Border terrier",
  "Kerry blue terrier",
  "Irish terrier",
  "Norfolk terrier",
  "Norwich terrier",
  "Yorkshire terrier",
  "wire-haired fox terrier",
  "Lakeland terrier",
  "Sealyham terrier, Sealyham",
  "Airedale, Airedale terrier",
  "cairn, cairn terrier",
  "Australian terrier",
  "Dandie Dinmont, Dandie Dinmont terrier",
  "Boston bull, Boston terrier",
  "miniature schnauzer",
  "giant schnauzer",
  "standard schnauzer",
  "Scotch terrier, Scottish terrier, Scottie",
  "Tibetan terrier, chrysanthemum dog",
  "silky terrier, Sydney silky",
  "soft-coated wheaten terrier",
  "West Highland white terrier",
  "Lhasa, Lhasa apso",
  "flat-coated retriever",
  "curly-coated retriever",
  "golden retriever",
  "Labrador retriever",
  "Chesapeake Bay retriever",
  "German short-haired pointer",
  "vizsla, Hungarian pointer",
  "English setter",
  "Irish setter, red setter",
  "Gordon setter",
  "Brittany spaniel",
  "clumber, clumber spaniel",
  "English springer, English springer spaniel",
  "Welsh springer spaniel",
  "cocker spaniel, English cocker spaniel, cocker",
  "Sussex spaniel",
  "Irish water spaniel",
  "kuvasz",
  "schipperke",
  "groenendael",
  "malinois",
  "briard",
  "kelpie",
  "komondor",
  "Old English sheepdog, bobtail",
  "Shetland sheepdog, Shetland sheep dog, Shetland",
  "collie",
  "Border collie",
  "Bouvier des Flandres, Bouviers des Flandres",
  "Rottweiler",
  "German shepherd, German shepherd dog, German police dog, alsatian",
  "Doberman, Doberman pinscher",
  "miniature pinscher",
  "Greater Swiss Mountain dog",
  "Bernese mountain dog",
  "Appenzeller",
  "EntleBucher",
  "boxer",
  "bull mastiff",
  "Tibetan mastiff",
  "French bulldog",
  "Great Dane",
  "Saint Bernard, St Bernard",
  "Eskimo dog, husky",
  "malamute, malemute, Alaskan malamute",
  "Siberian husky",
  "dalmatian, coach dog, carriage dog",
  "affenpinscher, monkey pinscher, monkey dog",
  "basenji",
  "pug, pug-dog",
  "Leonberg",
  "Newfoundland, Newfoundland dog",
  "Great Pyrenees",
  "Samoyed, Samoyede",
  "Pomeranian",
  "chow, chow chow",
  "keeshond",
  "Brabancon griffon",
  "Pembroke, Pembroke Welsh corgi",
  "Cardigan, Cardigan Welsh corgi",
  "toy poodle",
  "miniature poodle",
  "standard poodle",
  "Mexican hairless",
  "timber wolf, grey wolf, gray wolf, Canis lupus",
  "white wolf, Arctic wolf, Canis lupus tundrarum",
  "red wolf, maned wolf, Canis rufus, Canis niger",
  "coyote, prairie wolf, brush wolf, Canis latrans",
  "dingo, warrigal, warragal, Canis dingo",
  "dhole, Cuon alpinus",
  "African hunting dog, hyena dog, Cape hunting dog, Lycaon pictus",
  "hyena, hyaena",
  "red fox, Vulpes vulpes",
  "kit fox, Vulpes macrotis",
  "Arctic fox, white fox, Alopex lagopus",
  "grey fox, gray fox, Urocyon cinereoargenteus",
  "tabby, tabby cat",
  "tiger cat",
  "Persian cat",
  "Siamese cat, Siamese",
  "Egyptian cat",
  "cougar, puma, catamount, mountain lion, painter, panther, Felis concolor",
  "lynx, catamount",
  "leopard, Panthera pardus",
  "snow leopard, ounce, Panthera uncia",
  "jaguar, panther, Panthera onca, Felis onca",
  "lion, king of beasts, Panthera leo",
  "tiger, Panthera tigris",
  "cheetah, chetah, Acinonyx jubatus",
  "brown bear, bruin, Ursus arctos",
  "American black bear, black bear, Ursus americanus, Euarctos americanus",
  "ice bear, polar bear, Ursus Maritimus, Thalarctos maritimus",
  "sloth bear, Melursus ursinus, Ursus ursinus",
  "mongoose",
  "meerkat, mierkat",
  "tiger beetle",
  "ladybug, ladybeetle, lady beetle, ladybird, ladybird beetle",
  "ground beetle, carabid beetle",
  "long-horned beetle, longicorn, longicorn beetle",
  "leaf beetle, chrysomelid",
  "dung beetle",
  "rhinoceros beetle",
  "weevil",
  "fly",
  "bee",
  "ant, emmet, pismire",
  "grasshopper, hopper",
  "cricket",
  "walking stick, walkingstick, stick insect",
  "cockroach, roach",
  "mantis, mantid",
  "cicada, cicala",
  "leafhopper",
  "lacewing, lacewing fly",
  "dragonfly, darning needle, devil's darning needle, sewing needle, snake feeder, snake doctor, mosquito hawk, skeeter hawk",
  "damselfly",
  "admiral",
  "ringlet, ringlet butterfly",
  "monarch, monarch butterfly, milkweed butterfly, Danaus plexippus",
  "cabbage butterfly",
  "sulphur butterfly, sulfur butterfly",
  "lycaenid, lycaenid butterfly",
  "starfish, sea star",
  "sea urchin",
  "sea cucumber, holothurian",
  "wood rabbit, cottontail, cottontail rabbit",
  "hare",
  "Angora, Angora rabbit",
  "hamster",
  "porcupine, hedgehog",
  "fox squirrel, eastern fox squirrel, Sciurus niger",
  "marmot",
  "beaver",
  "guinea pig, Cavia cobaya",
  "sorrel",
  "zebra",
  "hog, pig, grunter, squealer, Sus scrofa",
  "wild boar, boar, Sus scrofa",
  "warthog",
  "hippopotamus, hippo, river horse, Hippopotamus amphibius",
  "ox",
  "water buffalo, water ox, Asiatic buffalo, Bubalus bubalis",
  "bison",
  "ram, tup",
  "bighorn, bighorn sheep, cimarron, Rocky Mountain bighorn, Rocky Mountain sheep, Ovis canadensis",
  "ibex, Capra ibex",
  "hartebeest",
  "impala, Aepyceros melampus",
  "gazelle",
  "Arabian camel, dromedary, Camelus dromedarius",
  "llama",
  "weasel",
  "mink",
  "polecat, fitch, foulmart, foumart, Mustela putorius",
  "black-footed ferret, ferret, Mustela nigripes",
  "otter",
  "skunk, polecat, wood pussy",
  "badger",
  "armadillo",
  "three-toed sloth, ai, Bradypus tridactylus",
  "orangutan, orang, orangutang, Pongo pygmaeus",
  "gorilla, Gorilla gorilla",
  "chimpanzee, chimp, Pan troglodytes",
  "gibbon, Hylobates lar",
  "siamang, Hylobates syndactylus, Symphalangus syndactylus",
  "guenon, guenon monkey",
  "patas, hussar monkey, Erythrocebus patas",
  "baboon",
  "macaque",
  "langur",
  "colobus, colobus monkey",
  "proboscis monkey, Nasalis larvatus",
  "marmoset",
  "capuchin, ringtail, Cebus capucinus",
  "howler monkey, howler",
  "titi, titi monkey",
  "spider monkey, Ateles geoffroyi",
  "squirrel monkey, Saimiri sciureus",
  "Madagascar cat, ring-tailed lemur, Lemur catta",
  "indri, indris, Indri indri, Indri brevicaudatus",
  "Indian elephant, Elephas maximus",
  "African elephant, Loxodonta africana",
  "lesser panda, red panda, panda, bear cat, cat bear, Ailurus fulgens",
  "giant panda, panda, panda bear, coon bear, Ailuropoda melanoleuca",
  "barracouta, snoek",
  "eel",
  "coho, cohoe, coho salmon, blue jack, silver salmon, Oncorhynchus kisutch",
  "rock beauty, Holocanthus tricolor",
  "anemone fish",
  "sturgeon",
  "gar, garfish, garpike, billfish, Lepisosteus osseus",
  "lionfish",
  "puffer, pufferfish, blowfish, globefish",
  "abacus",
  "abaya",
  "academic gown, academic robe, judge's robe",
  "accordion, piano accordion, squeeze box",
  "acoustic guitar",
  "aircraft carrier, carrier, flattop, attack aircraft carrier",
  "airliner",
  "airship, dirigible",
  "altar",
  "ambulance",
  "amphibian, amphibious vehicle",
  "analog clock",
  "apiary, bee house",
  "apron",
  "ashcan, trash can, garbage can, wastebin, ash bin, ash-bin, ashbin, dustbin, trash barrel, trash bin",
  "assault rifle, assault gun",
  "backpack, back pack, knapsack, packsack, rucksack, haversack",
  "bakery, bakeshop, bakehouse",
  "balance beam, beam",
  "balloon",
  "ballpoint, ballpoint pen, ballpen, Biro",
  "Band Aid",
  "banjo",
  "bannister, banister, balustrade, balusters, handrail",
  "barbell",
  "barber chair",
  "barbershop",
  "barn",
  "barometer",
  "barrel, cask",
  "barrow, garden cart, lawn cart, wheelbarrow",
  "baseball",
  "basketball",
  "bassinet",
  "bassoon",
  "bathing cap, swimming cap",
  "bath towel",
  "bathtub, bathing tub, bath, tub",
  "beach wagon, station wagon, wagon, estate car, beach waggon, station waggon, waggon",
  "beacon, lighthouse, beacon light, pharos",
  "beaker",
  "bearskin, busby, shako",
  "beer bottle",
  "beer glass",
  "bell cote, bell cot",
  "bib",
  "bicycle-built-for-two, tandem bicycle, tandem",
  "bikini, two-piece",
  "binder, ring-binder",
  "binoculars, field glasses, opera glasses",
  "birdhouse",
  "boathouse",
  "bobsled, bobsleigh, bob",
  "bolo tie, bolo, bola tie, bola",
  "bonnet, poke bonnet",
  "bookcase",
  "bookshop, bookstore, bookstall",
  "bottlecap",
  "bow",
  "bow tie, bow-tie, bowtie",
  "brass, memorial tablet, plaque",
  "brassiere, bra, bandeau",
  "breakwater, groin, groyne, mole, bulwark, seawall, jetty",
  "breastplate, aegis, egis",
  "broom",
  "bucket, pail",
  "buckle",
  "bulletproof vest",
  "bullet train, bullet",
  "butcher shop, meat market",
  "cab, hack, taxi, taxicab",
  "caldron, cauldron",
  "candle, taper, wax light",
  "cannon",
  "canoe",
  "can opener, tin opener",
  "cardigan",
  "car mirror",
  "carousel, carrousel, merry-go-round, roundabout, whirligig",
  "carpenter's kit, tool kit",
  "carton",
  "car wheel",
  "cash machine, cash dispenser, automated teller machine, automatic teller machine, automated teller, automatic teller, ATM",
  "cassette",
  "cassette player",
  "castle",
  "catamaran",
  "CD player",
  "cello, violoncello",
  "cellular telephone, cellular phone, cellphone, cell, mobile phone",
  "chain",
  "chainlink fence",
  "chain mail, ring mail, mail, chain armor, chain armour, ring armor, ring armour",
  "chain saw, chainsaw",
  "chest",
  "chiffonier, commode",
  "chime, bell, gong",
  "china cabinet, china closet",
  "Christmas stocking",
  "church, church building",
  "cinema, movie theater, movie theatre, movie house, picture palace",
  "cleaver, meat cleaver, chopper",
  "cliff dwelling",
  "cloak",
  "clog, geta, patten, sabot",
  "cocktail shaker",
  "coffee mug",
  "coffeepot",
  "coil, spiral, volute, whorl, helix",
  "combination lock",
  "computer keyboard, keypad",
  "confectionery, confectionary, candy store",
  "container ship, containership, container vessel",
  "convertible",
  "corkscrew, bottle screw",
  "cornet, horn, trumpet, trump",
  "cowboy boot",
  "cowboy hat, ten-gallon hat",
  "cradle",
  "crane",
  "crash helmet",
  "crate",
  "crib, cot",
  "Crock Pot",
  "croquet ball",
  "crutch",
  "cuirass",
  "dam, dike, dyke",
  "desk",
  "desktop computer",
  "dial telephone, dial phone",
  "diaper, nappy, napkin",
  "digital clock",
  "digital watch",
  "dining table, board",
  "dishrag, dishcloth",
  "dishwasher, dish washer, dishwashing machine",
  "disk brake, disc brake",
  "dock, dockage, docking facility",
  "dogsled, dog sled, dog sleigh",
  "dome",
  "doormat, welcome mat",
  "drilling platform, offshore rig",
  "drum, membranophone, tympan",
  "drumstick",
  "dumbbell",
  "Dutch oven",
  "electric fan, blower",
  "electric guitar",
  "electric locomotive",
  "entertainment center",
  "envelope",
  "espresso maker",
  "face powder",
  "feather boa, boa",
  "file, file cabinet, filing cabinet",
  "fireboat",
  "fire engine, fire truck",
  "fire screen, fireguard",
  "flagpole, flagstaff",
  "flute, transverse flute",
  "folding chair",
  "football helmet",
  "forklift",
  "fountain",
  "fountain pen",
  "four-poster",
  "freight car",
  "French horn, horn",
  "frying pan, frypan, skillet",
  "fur coat",
  "garbage truck, dustcart",
  "gasmask, respirator, gas helmet",
  "gas pump, gasoline pump, petrol pump, island dispenser",
  "goblet",
  "go-kart",
  "golf ball",
  "golfcart, golf cart",
  "gondola",
  "gong, tam-tam",
  "gown",
  "grand piano, grand",
  "greenhouse, nursery, glasshouse",
  "grille, radiator grille",
  "grocery store, grocery, food market, market",
  "guillotine",
  "hair slide",
  "hair spray",
  "half track",
  "hammer",
  "hamper",
  "hand blower, blow dryer, blow drier, hair dryer, hair drier",
  "hand-held computer, hand-held microcomputer",
  "handkerchief, hankie, hanky, hankey",
  "hard disc, hard disk, fixed disk",
  "harmonica, mouth organ, harp, mouth harp",
  "harp",
  "harvester, reaper",
  "hatchet",
  "holster",
  "home theater, home theatre",
  "honeycomb",
  "hook, claw",
  "hoopskirt, crinoline",
  "horizontal bar, high bar",
  "horse cart, horse-cart",
  "hourglass",
  "iPod",
  "iron, smoothing iron",
  "jack-o-lantern",
  "jean, blue jean, denim",
  "jeep, landrover",
  "jersey, T-shirt, tee shirt",
  "jigsaw puzzle",
  "jinrikisha, ricksha, rickshaw",
  "joystick",
  "kimono",
  "knee pad",
  "knot",
  "lab coat, laboratory coat",
  "ladle",
  "lampshade, lamp shade",
  "laptop, laptop computer",
  "lawn mower, mower",
  "lens cap, lens cover",
  "letter opener, paper knife, paperknife",
  "library",
  "lifeboat",
  "lighter, light, igniter, ignitor",
  "limousine, limo",
  "liner, ocean liner",
  "lipstick, lip rouge",
  "Loafer",
  "lotion",
  "loudspeaker, speaker, speaker unit, loudspeaker system, speaker system",
  "loupe, jeweler's loupe",
  "lumbermill, sawmill",
  "magnetic compass",
  "mailbag, postbag",
  "mailbox, letter box",
  "maillot",
  "maillot, tank suit",
  "manhole cover",
  "maraca",
  "marimba, xylophone",
  "mask",
  "matchstick",
  "maypole",
  "maze, labyrinth",
  "measuring cup",
  "medicine chest, medicine cabinet",
  "megalith, megalithic structure",
  "microphone, mike",
  "microwave, microwave oven",
  "military uniform",
  "milk can",
  "minibus",
  "miniskirt, mini",
  "minivan",
  "missile",
  "mitten",
  "mixing bowl",
  "mobile home, manufactured home",
  "Model T",
  "modem",
  "monastery",
  "monitor",
  "moped",
  "mortar",
  "mortarboard",
  "mosque",
  "mosquito net",
  "motor scooter, scooter",
  "mountain bike, all-terrain bike, off-roader",
  "mountain tent",
  "mouse, computer mouse",
  "mousetrap",
  "moving van",
  "muzzle",
  "nail",
  "neck brace",
  "necklace",
  "nipple",
  "notebook, notebook computer",
  "obelisk",
  "oboe, hautboy, hautbois",
  "ocarina, sweet potato",
  "odometer, hodometer, mileometer, milometer",
  "oil filter",
  "organ, pipe organ",
  "oscilloscope, scope, cathode-ray oscilloscope, CRO",
  "overskirt",
  "oxcart",
  "oxygen mask",
  "packet",
  "paddle, boat paddle",
  "paddlewheel, paddle wheel",
  "padlock",
  "paintbrush",
  "pajama, pyjama, pj's, jammies",
  "palace",
  "panpipe, pandean pipe, syrinx",
  "paper towel",
  "parachute, chute",
  "parallel bars, bars",
  "park bench",
  "parking meter",
  "passenger car, coach, carriage",
  "patio, terrace",
  "pay-phone, pay-station",
  "pedestal, plinth, footstall",
  "pencil box, pencil case",
  "pencil sharpener",
  "perfume, essence",
  "Petri dish",
  "photocopier",
  "pick, plectrum, plectron",
  "pickelhaube",
  "picket fence, paling",
  "pickup, pickup truck",
  "pier",
  "piggy bank, penny bank",
  "pill bottle",
  "pillow",
  "ping-pong ball",
  "pinwheel",
  "pirate, pirate ship",
  "pitcher, ewer",
  "plane, carpenter's plane, woodworking plane",
  "planetarium",
  "plastic bag",
  "plate rack",
  "plow, plough",
  "plunger, plumber's helper",
  "Polaroid camera, Polaroid Land camera",
  "pole",
  "police van, police wagon, paddy wagon, patrol wagon, wagon, black Maria",
  "poncho",
  "pool table, billiard table, snooker table",
  "pop bottle, soda bottle",
  "pot, flowerpot",
  "potter's wheel",
  "power drill",
  "prayer rug, prayer mat",
  "printer",
  "prison, prison house",
  "projectile, missile",
  "projector",
  "puck, hockey puck",
  "punching bag, punch bag, punching ball, punchball",
  "purse",
  "quill, quill pen",
  "quilt, comforter, comfort, puff",
  "racer, race car, racing car",
  "racket, racquet",
  "radiator",
  "radio, wireless",
  "radio telescope, radio reflector",
  "rain barrel",
  "recreational vehicle, RV, R.V.",
  "reel",
  "reflex camera",
  "refrigerator, icebox",
  "remote control, remote",
  "restaurant, eating house, eating place, eatery",
  "revolver, six-gun, six-shooter",
  "rifle",
  "rocking chair, rocker",
  "rotisserie",
  "rubber eraser, rubber, pencil eraser",
  "rugby ball",
  "rule, ruler",
  "running shoe",
  "safe",
  "safety pin",
  "saltshaker, salt shaker",
  "sandal",
  "sarong",
  "sax, saxophone",
  "scabbard",
  "scale, weighing machine",
  "school bus",
  "schooner",
  "scoreboard",
  "screen, CRT screen",
  "screw",
  "screwdriver",
  "seat belt, seatbelt",
  "sewing machine",
  "shield, buckler",
  "shoe shop, shoe-shop, shoe store",
  "shoji",
  "shopping basket",
  "shopping cart",
  "shovel",
  "shower cap",
  "shower curtain",
  "ski",
  "ski mask",
  "sleeping bag",
  "slide rule, slipstick",
  "sliding door",
  "slot, one-armed bandit",
  "snorkel",
  "snowmobile",
  "snowplow, snowplough",
  "soap dispenser",
  "soccer ball",
  "sock",
  "solar dish, solar collector, solar furnace",
  "sombrero",
  "soup bowl",
  "space bar",
  "space heater",
  "space shuttle",
  "spatula",
  "speedboat",
  "spider web, spider's web",
  "spindle",
  "sports car, sport car",
  "spotlight, spot",
  "stage",
  "steam locomotive",
  "steel arch bridge",
  "steel drum",
  "stethoscope",
  "stole",
  "stone wall",
  "stopwatch, stop watch",
  "stove",
  "strainer",
  "streetcar, tram, tramcar, trolley, trolley car",
  "stretcher",
  "studio couch, day bed",
  "stupa, tope",
  "submarine, pigboat, sub, U-boat",
  "suit, suit of clothes",
  "sundial",
  "sunglass",
  "sunglasses, dark glasses, shades",
  "sunscreen, sunblock, sun blocker",
  "suspension bridge",
  "swab, swob, mop",
  "sweatshirt",
  "swimming trunks, bathing trunks",
  "swing",
  "switch, electric switch, electrical switch",
  "syringe",
  "table lamp",
  "tank, army tank, armored combat vehicle, armoured combat vehicle",
  "tape player",
  "teapot",
  "teddy, teddy bear",
  "television, television system",
  "tennis ball",
  "thatch, thatched roof",
  "theater curtain, theatre curtain",
  "thimble",
  "thresher, thrasher, threshing machine",
  "throne",
  "tile roof",
  "toaster",
  "tobacco shop, tobacconist shop, tobacconist",
  "toilet seat",
  "torch",
  "totem pole",
  "tow truck, tow car, wrecker",
  "toyshop",
  "tractor",
  "trailer truck, tractor trailer, trucking rig, rig, articulated lorry, semi",
  "tray",
  "trench coat",
  "tricycle, trike, velocipede",
  "trimaran",
  "tripod",
  "triumphal arch",
  "trolleybus, trolley coach, trackless trolley",
  "trombone",
  "tub, vat",
  "turnstile",
  "typewriter keyboard",
  "umbrella",
  "unicycle, monocycle",
  "upright, upright piano",
  "vacuum, vacuum cleaner",
  "vase",
  "vault",
  "velvet",
  "vending machine",
  "vestment",
  "viaduct",
  "violin, fiddle",
  "volleyball",
  "waffle iron",
  "wall clock",
  "wallet, billfold, notecase, pocketbook",
  "wardrobe, closet, press",
  "warplane, military plane",
  "washbasin, handbasin, washbowl, lavabo, wash-hand basin",
  "washer, automatic washer, washing machine",
  "water bottle",
  "water jug",
  "water tower",
  "whiskey jug",
  "whistle",
  "wig",
  "window screen",
  "window shade",
  "Windsor tie",
  "wine bottle",
  "wing",
  "wok",
  "wooden spoon",
  "wool, woolen, woollen",
  "worm fence, snake fence, snake-rail fence, Virginia fence",
  "wreck",
  "yawl",
  "yurt",
  "web site, website, internet site, site",
  "comic book",
  "crossword puzzle, crossword",
  "street sign",
  "traffic light, traffic signal, stoplight",
  "book jacket, dust cover, dust jacket, dust wrapper",
  "menu",
  "plate",
  "guacamole",
  "consomme",
  "hot pot, hotpot",
  "trifle",
  "ice cream, icecream",
  "ice lolly, lolly, lollipop, popsicle",
  "French loaf",
  "bagel, beigel",
  "pretzel",
  "cheeseburger",
  "hotdog, hot dog, red hot",
  "mashed potato",
  "head cabbage",
  "broccoli",
  "cauliflower",
  "zucchini, courgette",
  "spaghetti squash",
  "acorn squash",
  "butternut squash",
  "cucumber, cuke",
  "artichoke, globe artichoke",
  "bell pepper",
  "cardoon",
  "mushroom",
  "Granny Smith",
  "strawberry",
  "orange",
  "lemon",
  "fig",
  "pineapple, ananas",
  "banana",
  "jackfruit, jak, jack",
  "custard apple",
  "pomegranate",
  "hay",
  "carbonara",
  "chocolate sauce, chocolate syrup",
  "dough",
  "meat loaf, meatloaf",
  "pizza, pizza pie",
  "potpie",
  "burrito",
  "red wine",
  "espresso",
  "cup",
  "eggnog",
  "alp",
  "bubble",
  "cliff, drop, drop-off",
  "coral reef",
  "geyser",
  "lakeside, lakeshore",
  "promontory, headland, head, foreland",
  "sandbar, sand bar",
  "seashore, coast, seacoast, sea-coast",
  "valley, vale",
  "volcano",
  "ballplayer, baseball player",
  "groom, bridegroom",
  "scuba diver",
  "rapeseed",
  "daisy",
  "yellow lady's slipper, yellow lady-slipper, Cypripedium calceolus, Cypripedium parviflorum",
  "corn",
  "acorn",
  "hip, rose hip, rosehip",
  "buckeye, horse chestnut, conker",
  "coral fungus",
  "agaric",
  "gyromitra",
  "stinkhorn, carrion fungus",
  "earthstar",
  "hen-of-the-woods, hen of the woods, Polyporus frondosus, Grifola frondosa",
  "bolete",
  "ear, spike, capitulum",
  "toilet tissue, toilet paper, bathroom tissue"
};
static int32_t label_offset3[TIVX_DL_POST_PROC_MAX_NUM_CLASSNAMES] = {0};

vx_status app_modules_capture_dl_classification_display_test(vx_int32 argc, vx_char* argv[])
{
    vx_status status = VX_FAILURE;
    GraphObj graph;
    NodeObj *capture_node = NULL, *tee_node = NULL, *viss_node = NULL, *ldc_node = NULL, *msc_node = NULL, *display_node = NULL, *aewb_node = NULL;
    TIOVXCaptureNodeCfg capture_cfg;
    TIOVXTeeNodeCfg tee_cfg;
    TIOVXVissNodeCfg viss_cfg;
    TIOVXLdcNodeCfg ldc_cfg;
    TIOVXMultiScalerNodeCfg msc_cfg;
    BufPool *in_buf_pool = NULL;
    Buf *inbuf = NULL;
    #if defined(SOC_AM62A) && defined(TARGET_OS_QNX)
    BufPool *out_buf_pool = NULL; 
    Buf *outbuf = NULL;
    #endif
    int32_t frame_count = 0;
    Pad *input_pad =NULL, *output_pad[4] = {NULL, NULL, NULL, NULL};  
   
    status = tiovx_modules_initialize_graph(&graph);
    graph.schedule_mode = VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO;

    /* Capture */
    tiovx_capture_init_cfg(&capture_cfg);

    sprintf(capture_cfg.sensor_name, SENSOR_NAME);
    capture_cfg.ch_mask = 1;
    capture_cfg.enable_error_detection = 0;
    #if defined(SOC_AM62A) && defined(TARGET_OS_QNX)
    sprintf(capture_cfg.target_string, TIVX_TARGET_CAPTURE2);
    #endif

    capture_node = tiovx_modules_add_node(&graph, TIOVX_CAPTURE, (void *)&capture_cfg);
    input_pad = &capture_node->srcs[0];

    /* TEE */
    tiovx_tee_init_cfg(&tee_cfg);

    tee_cfg.peer_pad = &capture_node->srcs[0];
    tee_cfg.num_outputs = 2;

    tee_node = tiovx_modules_add_node(&graph, TIOVX_TEE, (void *)&tee_cfg);

    tee_node->srcs[0].bufq_depth = 4; /* This must be greater than 3 */
    input_pad = &tee_node->srcs[0]; // This is for enqueing

    /* VISS */
    tiovx_viss_init_cfg(&viss_cfg);

    sprintf(viss_cfg.sensor_name, SENSOR_NAME);
    snprintf(viss_cfg.dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", DCC_VISS);
    viss_cfg.width = VISS_INPUT_WIDTH;
    viss_cfg.height = VISS_INPUT_HEIGHT;
    sprintf(viss_cfg.target_string, TIVX_TARGET_VPAC_VISS1);

    viss_cfg.enable_h3a_pad = vx_true_e;
    viss_cfg.input_cfg.params.format[0].pixel_container = TIVX_RAW_IMAGE_16_BIT;
    viss_cfg.input_cfg.params.format[0].msb = 11;

    viss_node = tiovx_modules_add_node(&graph, TIOVX_VISS, (void *)&viss_cfg);

    output_pad[0] = &viss_node->srcs[0];
    output_pad[1] = &viss_node->srcs[1];

    /* AEWB */

    TIOVXAewbNodeCfg aewb_cfg;
    tiovx_aewb_init_cfg(&aewb_cfg);

    sprintf(aewb_cfg.sensor_name, SENSOR_NAME);
    aewb_cfg.ch_mask = 1;
    aewb_cfg.awb_mode = ALGORITHMS_ISS_AWB_AUTO;
    aewb_cfg.awb_num_skip_frames = 0;
    aewb_cfg.ae_num_skip_frames  = 0;

    aewb_node = tiovx_modules_add_node(&graph, TIOVX_AEWB, (void *)&aewb_cfg);


    /* LDC */
    tiovx_ldc_init_cfg(&ldc_cfg);

    sprintf(ldc_cfg.sensor_name, SENSOR_NAME);
    snprintf(ldc_cfg.dcc_config_file, TIVX_FILEIO_FILE_PATH_LENGTH, "%s", DCC_LDC);
    ldc_cfg.ldc_mode = TIOVX_MODULE_LDC_OP_MODE_DCC_DATA;

    ldc_cfg.input_cfg.color_format = VX_DF_IMAGE_NV12;
    ldc_cfg.input_cfg.width = LDC_INPUT_WIDTH;
    ldc_cfg.input_cfg.height = LDC_INPUT_HEIGHT;

    ldc_cfg.output_cfgs[0].color_format = VX_DF_IMAGE_NV12;
    ldc_cfg.output_cfgs[0].width = LDC_OUTPUT_WIDTH;
    ldc_cfg.output_cfgs[0].height = LDC_OUTPUT_HEIGHT;

    ldc_cfg.init_x = 0;
    ldc_cfg.init_y = 0;
    ldc_cfg.table_width = LDC_TABLE_WIDTH;
    ldc_cfg.table_height = LDC_TABLE_HEIGHT;
    ldc_cfg.ds_factor = LDC_DS_FACTOR;
    ldc_cfg.out_block_width = LDC_BLOCK_WIDTH;
    ldc_cfg.out_block_height = LDC_BLOCK_HEIGHT;
    ldc_cfg.pixel_pad = LDC_PIXEL_PAD;

    sprintf(ldc_cfg.target_string, TIVX_TARGET_VPAC_LDC1);

    ldc_node = tiovx_modules_add_node(&graph, TIOVX_LDC, (void *)&ldc_cfg);

    /* DL Pre Proc */

    TIOVXDLPreProcNodeCfg dl_pre_proc_cfg;
    NodeObj *dl_pre_proc_node;

    tiovx_dl_pre_proc_init_cfg(&dl_pre_proc_cfg);

    dl_pre_proc_cfg.io_config_path = TIDL_IO_CONFIG_FILE_PATH;
    dl_pre_proc_cfg.params.tensor_format = 1; //BGR

    dl_pre_proc_node = tiovx_modules_add_node(&graph, TIOVX_DL_PRE_PROC, (void *)&dl_pre_proc_cfg);

    /* TIDL */
    
    TIOVXTIDLNodeCfg tidl_cfg;
    NodeObj *tidl_node;
    tiovx_tidl_init_cfg(&tidl_cfg);
    tidl_cfg.io_config_path = TIDL_IO_CONFIG_FILE_PATH;
    tidl_cfg.network_path = TIDL_NETWORK_FILE_PATH;
    tidl_node = tiovx_modules_add_node(&graph, TIOVX_TIDL, (void *)&tidl_cfg);

    /* DL POST PROC*/

    TIOVXDLPostProcNodeCfg dl_post_proc_cfg;
    NodeObj *dl_post_proc_node;
    tiovx_dl_post_proc_init_cfg(&dl_post_proc_cfg);
    dl_post_proc_cfg.width = POST_PROC_OUT_WIDTH;
    dl_post_proc_cfg.height = POST_PROC_OUT_HEIGHT;
    //additional line added
    dl_post_proc_cfg.num_input_tensors=1;
    dl_post_proc_cfg.params.task_type = TIVX_DL_POST_PROC_CLASSIFICATION_TASK_TYPE;
    dl_post_proc_cfg.params.oc_prms.labelOffset = 1;
    dl_post_proc_cfg.params.oc_prms.num_top_results = 5;
    dl_post_proc_cfg.params.oc_prms.classnames= malloc(TIVX_DL_POST_PROC_MAX_NUM_CLASSNAMES * TIVX_DL_POST_PROC_MAX_SIZE_CLASSNAME * sizeof(char));
    for(uint32_t i = 0; i < 1001; i++)
    {
        strcpy(dl_post_proc_cfg.params.oc_prms.classnames[i], imgnet_labels1[i]);
    }

    dl_post_proc_cfg.io_config_path = TIDL_IO_CONFIG_FILE_PATH;
    dl_post_proc_node = tiovx_modules_add_node(&graph, TIOVX_DL_POST_PROC, (void *)&dl_post_proc_cfg);

    /* MSC0*/
    tiovx_multi_scaler_init_cfg(&msc_cfg);

    msc_cfg.color_format = VX_DF_IMAGE_NV12;
    msc_cfg.num_outputs = 2;

    msc_cfg.input_cfg.width = LDC_OUTPUT_WIDTH;
    msc_cfg.input_cfg.height = LDC_OUTPUT_HEIGHT;

    // This is for pre proc
    msc_cfg.output_cfgs[0].width = LDC_OUTPUT_WIDTH / 2;
    msc_cfg.output_cfgs[0].height = LDC_OUTPUT_HEIGHT / 2;

    msc_cfg.output_cfgs[1].width = POST_PROC_OUT_WIDTH;
    msc_cfg.output_cfgs[1].height = POST_PROC_OUT_HEIGHT;

    sprintf(msc_cfg.target_string, TIVX_TARGET_VPAC_MSC1);
    tiovx_multi_scaler_module_crop_params_init(&msc_cfg);

    msc_node = tiovx_modules_add_node(&graph, TIOVX_MULTI_SCALER, (void *)&msc_cfg);

    /* MSC1*/
    TIOVXMultiScalerNodeCfg msc1_cfg;
    NodeObj *msc1_node;
    TIOVXDLPreProcNodeCfg *tempCfg = (TIOVXDLPreProcNodeCfg *)dl_pre_proc_node->node_cfg;

    tiovx_multi_scaler_init_cfg(&msc1_cfg);

    msc1_cfg.color_format = VX_DF_IMAGE_NV12;
    msc1_cfg.num_outputs = 1;

    msc1_cfg.input_cfg.width = LDC_OUTPUT_WIDTH / 2;
    msc1_cfg.input_cfg.height = LDC_OUTPUT_HEIGHT / 2;

    msc1_cfg.output_cfgs[0].width = tempCfg->input_cfg.width;
    msc1_cfg.output_cfgs[0].height = tempCfg->input_cfg.height;

    sprintf(msc1_cfg.target_string, TIVX_TARGET_VPAC_MSC1);
    tiovx_multi_scaler_module_crop_params_init(&msc1_cfg);

    msc1_node = tiovx_modules_add_node(&graph, TIOVX_MULTI_SCALER, (void *)&msc1_cfg);

    /* Link PADS */
    tiovx_modules_link_pads(&tee_node->srcs[1], &viss_node->sinks[0]);
    tiovx_modules_link_pads(&viss_node->srcs[1], &aewb_node->sinks[0]); 
    tiovx_modules_link_pads(&viss_node->srcs[0], &ldc_node->sinks[0]);
    tiovx_modules_link_pads(&ldc_node->srcs[0],&msc_node->sinks[0]);
    tiovx_modules_link_pads(&msc_node->srcs[0],&msc1_node->sinks[0]);
    tiovx_modules_link_pads(&msc1_node->srcs[0],&dl_pre_proc_node->sinks[0]);
    tiovx_modules_link_pads(&dl_pre_proc_node->srcs[0],&tidl_node->sinks[0]);
    tiovx_modules_link_pads(&tidl_node->srcs[0], &dl_post_proc_node->sinks[0]); // Tensor pad
    tiovx_modules_link_pads(&msc_node->srcs[1], &dl_post_proc_node->sinks[1]); // Image Pad

    /* Connect the floating node to Display */
     status = tiovx_modules_verify_graph(&graph);

    #if defined(SOC_AM62A) && defined(TARGET_OS_QNX)
    in_buf_pool = tee_node->srcs[0].buf_pool;
    out_buf_pool = dl_post_proc_node->srcs[0].buf_pool;
    for (int32_t i = 0; i < in_buf_pool->bufq_depth; i++)
    {
        inbuf = tiovx_modules_acquire_buf(in_buf_pool);
        tiovx_modules_enqueue_buf(inbuf);
    }

    for (int32_t i=0; i< out_buf_pool->bufq_depth;i++)
    { 
        outbuf = tiovx_modules_acquire_buf(out_buf_pool);
        tiovx_modules_enqueue_buf(outbuf);
    }

    while (frame_count++ < NUM_ITERATIONS)
    {
        inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
        tiovx_modules_enqueue_buf(inbuf);
        outbuf = tiovx_modules_dequeue_buf(out_buf_pool);
        /* function to render screen support using QNX on AM62A SOC */
        qnx_display_render_buf(outbuf);
        tiovx_modules_enqueue_buf(outbuf);
    }

    for (int32_t i = 0; i < 2; i++)
    {
        inbuf = tiovx_modules_dequeue_buf(in_buf_pool);
        tiovx_modules_release_buf(inbuf);
    }
 
    for(int32_t i =0; i< out_buf_pool->bufq_depth; i++)
    {
        outbuf = tiovx_modules_dequeue_buf(out_buf_pool);
        tiovx_modules_release_buf(outbuf);
    }

    tiovx_modules_clean_graph(&graph);
    free(dl_post_proc_cfg.params.oc_prms.classnames);

    return status;
    #endif
}
