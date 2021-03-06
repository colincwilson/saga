/* coding: utf-8 */
/* Saga - Un transcriptor fonético para el idioma español
 *
 * Copyright (C) 1993-2009  Albino Nogueiras Rodríguez y José B. Mariño
 *       TALP - Universitat Politècnica de Catalunya, ESPAÑA
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<limits.h>
#include	"Util.h"
#include    "PosixCompat.h"
#include	"LisUdf.h"
#include	"Saga.h"

/*
 * This is a version of the public domain getopt implementation by
 * Henry Spencer originally posted to net.sources.
 *
 * This is in the public domain.
 */

char *saga_optarg; /* Global argument pointer. */
int saga_optind = 0; /* Global argv index. */

static int saga_getopt(int argc, char *const *argv, const char *ostr)
{
        static char *scan = NULL; /* Private scan pointer. */
	char c;
	char *place;

	saga_optarg = NULL;

	if (!scan || *scan == '\0') {
		if (saga_optind == 0)
			saga_optind++;

		if (saga_optind >= argc || argv[saga_optind][0] != '-' || argv[saga_optind][1] == '\0')
			return EOF;
		if (argv[saga_optind][1] == '-' && argv[saga_optind][2] == '\0') {
			saga_optind++;
			return EOF;
		}

		scan = argv[saga_optind]+1;
		saga_optind++;
	}

	c = *scan++;
	place = strchr(ostr, c);

	if (!place || c == ':') {
		fprintf(stderr, "%s: unknown option -%c\n", argv[0], c);
		return '?';
	}

	place++;
	if (*place == ':') {
		if (*scan != '\0') {
			saga_optarg = scan;
			scan = NULL;
		} else if( saga_optind < argc ) {
			saga_optarg = argv[saga_optind];
			saga_optind++;
		} else {
			fprintf(stderr, "%s: option requires argument -%c\n", argv[0], c);
			return ':';
		}
	}

	return c;
}

/* Public domain function ends here */


static void EmpleoSaga(char **ArgV);
static int OpcSaga(int ArgC, char **ArgV,
                   SagaEngine *engine,
                   char **NomIn, char **NomOut, char **NomErr);


int main(int ArgC, char *ArgV[])
{
    char *NomIn = NULL, *NomOut = NULL, *NomErr = NULL;
    SagaEngine engine;
    int read_status;

    SagaEngine_Initialize(&engine);
    /*
     * Analizamos la linea de comandos.
     */
    if (OpcSaga(ArgC, ArgV, &engine, &NomIn, &NomOut, &NomErr) < 0)
    {
        EmpleoSaga(ArgV);
        SagaEngine_Clear(&engine);
        if (NomIn != NULL)
            free(NomIn);
        if (NomOut != NULL)
            free(NomOut);
        if (NomErr != NULL)
            free(NomErr);
        return EXIT_FAILURE;
    }

    if (SagaEngine_OpenErrorFile(&engine, NomErr) < 0)
    {
        SagaEngine_Clear(&engine);
        if (NomIn != NULL)
            free(NomIn);
        if (NomOut != NULL)
            free(NomOut);
        if (NomErr != NULL)
            free(NomErr);
        return EXIT_FAILURE;
    }

    if (SagaEngine_InputFromFileName(&engine, NomIn) < 0)
    {
        SagaEngine_Clear(&engine);
        if (NomIn != NULL)
            free(NomIn);
        if (NomOut != NULL)
            free(NomOut);
        if (NomErr != NULL)
            free(NomErr);
        return EXIT_FAILURE;
    }

    if (SagaEngine_OpenOutputFiles(&engine, NomOut) < 0)
    {
        SagaEngine_Clear(&engine);
        free(NomIn);
        if (NomOut != NULL)
            free(NomOut);
        if (NomErr != NULL)
            free(NomErr);
        return EXIT_FAILURE;
    }

    if (SagaEngine_Prepare(&engine) < 0)
    {
        SagaEngine_Clear(&engine);
        free(NomIn);
        if (NomOut != NULL)
            free(NomOut);
        if (NomErr != NULL)
            free(NomErr);
        return EXIT_FAILURE;
    }

    while (1)
    {
        read_status = SagaEngine_ReadText(&engine);
        if (read_status == -2)
        {
            SagaEngine_Refresh(&engine);
            SagaEngine_Clear(&engine);
            free(NomIn);
            if (NomOut != NULL)
                free(NomOut);
            if (NomErr != NULL)
                free(NomErr);
            return EXIT_FAILURE;
        }
        if (SagaEngine_Transcribe(&engine) < 0)
        {
            SagaEngine_Refresh(&engine);
            SagaEngine_Clear(&engine);
            free(NomIn);
            if (NomOut != NULL)
                free(NomOut);
            if (NomErr != NULL)
                free(NomErr);
            return EXIT_FAILURE;
        }

        if (SagaEngine_WriteOutputFiles(&engine) < 0)
        {
            SagaEngine_Refresh(&engine);
            continue;
        }

        if (SagaEngine_WriteErrorWords(&engine) < 0)
        {
            SagaEngine_Refresh(&engine);
            SagaEngine_Clear(&engine);
            free(NomIn);
            if (NomOut != NULL)
                free(NomOut);
            if (NomErr != NULL)
                free(NomErr);
            return EXIT_FAILURE;
        }
        if (read_status == -1)
        {
            /* End of file */
            break;
        }
    }

    SagaEngine_CloseInput(&engine);
    free(NomIn);
    if (NomOut != NULL)
        free(NomOut);
    if (NomErr != NULL)
        free(NomErr);
    SagaEngine_Clear(&engine);
    return EXIT_SUCCESS;
}


/***********************************************************************
 * OpcSaga - Analiza las opciones de la linea de comandos
 **********************************************************************/

static int OpcSaga(int ArgC,    /* No. argumentos linea de comandos             */
                   char **ArgV, /* Argumentos linea de comandos                 */
                   SagaEngine *engine, char **NomIn,    /* Nombre del fichero de entrada */
                   char **NomOut,       /* Nombre de los ficheros de salida             */
                   char **NomErr        /* Nombre del fichero de error */
    )
{
    int Opcion;
    size_t i;
    char **Matriz;

    int hay_salida = 0;
    *NomOut = NULL;
    *NomIn = NULL;
    *NomErr = NULL;

    while ((Opcion =
            saga_getopt(ArgC, ArgV, "abd:L:t:T:x:g:v:c:l:e:fFpysSM:Y:")) != -1)
    {
        switch (Opcion)
        {
        case 'L':
            if (SagaEngine_SetParamsFromVariant(engine, saga_optarg) < 0)
            {
                return -1;
            }
            break;
        case 'a':
            engine->TrnPalAis = 1;
            engine->TrnLinAis = 1;
            break;
        case 'b':
            engine->TrnLinAis = 1;
            break;
        case 'd':
            engine->FicDicExc = saga_optarg;
            break;
        case 't':
            engine->FicTrnFon = saga_optarg;
            break;
        case 'T':
            engine->FicTrnPal = saga_optarg;
            break;
        case 'x':
            engine->FicDicSust = saga_optarg;
            break;
        case 'g':
            engine->FicDicGrp = saga_optarg;
            break;
        case 'v':
            engine->FicNovVoc = saga_optarg;
            break;
        case 'c':
            engine->FicNovCons = saga_optarg;
            break;
        case 'l':
            engine->FicNovFon = saga_optarg;
            break;
        case 'e':
            *NomErr = saga_strdup(saga_optarg);
            break;
        case 'f':
            SagaEngine_EnableFonOutput(engine, 1);
            hay_salida = 1;
            break;
        case 'F':
            SagaEngine_EnableFnmOutput(engine, 1);
            hay_salida = 1;
            break;
        case 'p':
            SagaEngine_EnableFnmPalOutput(engine, 1);
            hay_salida = 1;
            break;
        case 'y':
            SagaEngine_EnableSefoOutput(engine, 1);
            hay_salida = 1;
            break;
        case 'Y':
            Matriz = MatStr(saga_optarg);
            engine->StrIniPal = engine->StrFinPal = Matriz[0];
            if (Matriz[1] != NULL)
                engine->StrFinPal = Matriz[1];
            free(Matriz);       /* I have stolen the inner pointers */
            break;
        case 's':
            SagaEngine_EnableSemOutput(engine, 1);
            break;
        case 'S':
            engine->ConSil = 1;
            break;
        case 'M':
            for (i = 0; i < strlen(saga_optarg); i++)
            {
                switch (saga_optarg[i])
                {
                case ' ':
                    continue;
                case 'S':
                    SagaEngine_Opt_Seseo(engine, 1);
                    break;
                case 'X':
                    SagaEngine_Opt_X_KS(engine, 1);
                    break;
                case 'H':
                    SagaEngine_Opt_SAspInc(engine, 1);
                    break;
                case 'h':
                    SagaEngine_Opt_SAspCond(engine, 1);
                    break;
                case 'K':
                    SagaEngine_Opt_SC_KS(engine, 1);
                    break;
                case 'A':
                    SagaEngine_Opt_BDG_Andes(engine, 1);
                    break;
                case 'N':
                    SagaEngine_Opt_NVelar(engine, 1);
                    break;
                case 'M':
                    SagaEngine_Opt_NasalVelar(engine, 1);
                    break;
                case 'P':
                    SagaEngine_Opt_ArchImpl(engine, 1);
                    break;
                case 'y':
                    SagaEngine_Opt_YVocal(engine, 1);
                    break;
                case 'R':
                    SagaEngine_Opt_RImpl(engine, 1);
                    break;
                case '@':
                    SagaEngine_Opt_GrupoSil(engine, 1);
                    break;
                case ':':
                    SagaEngine_Opt_MarcaImpl(engine, 1);
                    break;
                case '_':
                    SagaEngine_Opt_VocalPTON(engine, 1);
                    break;
                case '.':
                    SagaEngine_Opt_IniFinPal(engine, 1);
                    break;
                case '~':
                    SagaEngine_Opt_VocalNasal(engine, 1);
                    break;
                case 'C':
                    SagaEngine_Opt_OclusExpl(engine, 1);
                    break;
                case 'E':
                    switch (saga_optarg[++i])
                    {
                    case 'b':
                        SagaEngine_Opt_ElimB(engine, 1);
                        break;
                    case 'd':
                        SagaEngine_Opt_ElimD(engine, 1);
                        break;
                    case 'g':
                        SagaEngine_Opt_ElimG(engine, 1);
                        break;
                    default:
                        fprintf(stderr, "Clave desconocida \"%c\"\n",
                                saga_optarg[i + 1]);
                        return -1;
                        break;
                    }
                    break;
                default:
                    fprintf(stderr,
                            "Clave de modificacion desconocida \"%c\"\n",
                            saga_optarg[i]);
                    return -1;
                }
            }
            break;
        case '?':
            return -1;
        }
    }

    if (ArgC == saga_optind)
    {
        return -1;
    }

    *NomIn = saga_strdup(ArgV[saga_optind]);

    saga_optind++;
    if (ArgC > saga_optind)
    {
        *NomOut = saga_strdup(ArgV[saga_optind]);
    }
    else
    {
        *NomOut = saga_strdup("-");
    }

    if (*NomErr == NULL)
    {
        *NomErr = saga_strdup("-");
    }

    /*
     * La transcripcion por defecto es en alofonos (.fon);
     */
    if (!hay_salida)
    {
        SagaEngine_EnableFonOutput(engine, 1);
    }

    return 0;
}

/***********************************************************************
 * EmpleoSaga - Indica el empleo correcto de Saga
 **********************************************************************/

static void EmpleoSaga(char **ArgV)
{
    fprintf(stderr, "Empleo:\n");
    fprintf(stderr, "    %s [opciones] (TxtIn | -) [NomOut]\n", ArgV[0]);
    fprintf(stderr, "    %s -L castilla (TxtIn | -) [NomOut]\n", ArgV[0]);
    fprintf(stderr, "donde:\n");
    fprintf(stderr, "    TxtIn:    Texto ortografico de entrada\n");
    fprintf(stderr, "    NomOut:    Nombre, sin ext., ficheros de salida\n");
    fprintf(stderr, "y opciones puede ser:\n");
    fprintf(stderr, "    -L dialecto: Establece todas las opciones para un dialecto (argentina,\n");
    fprintf(stderr, "                 castilla, chile, colombia, mexico, peru o venezuela)\n");
   fprintf(stderr, "    -f        : Transcripcion fonetica (ext. .fon)\n");
    fprintf(stderr, "    -F        : Transcripcion en fonemas (ext. .fnm)\n");
    fprintf(stderr,
            "    -p        : Transcripcion en fonemas por palabras (ext. .fnp)\n");
    fprintf(stderr,
            "    -s        : Transcripcion en semisilabas (ext. .sem)\n");
    fprintf(stderr,
            "    -y        : Transcripcion en semifonemas (ext. .sef)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    -a        : Transcribir palabras aisladas\n");
    fprintf(stderr,
            "    -b        : Transcribir cada línea de forma aislada\n");
    fprintf(stderr, "    -S        : Conservar los silencios en la salida\n");
    fprintf(stderr,
            "    -Y ExtPal[,FinPal] : Marca de inicio y/o final de palabra\n");
    fprintf(stderr, "\n");
    fprintf(stderr,
            "    -d DicExc : Diccionario de excepciones ortograficas\n");
    fprintf(stderr,
            "    -T TrnPal : Diccionario de transcripcion de palabras\n");
    fprintf(stderr,
            "    -t TrnFon : Diccionario de transcripcion de grafemas\n");
    fprintf(stderr,
            "    -x DicSus : Diccionario de substitucion de fonemas\n");
    fprintf(stderr,
            "    -g DicGrp : Diccionario de substitucion de grupos foneticos\n");
    fprintf(stderr,
            "    -v NovVoc : Lista de grafemas vocalicos introducidos\n");
    fprintf(stderr,
            "    -c NovCns : Lista de grafemas consonanticos introducidos\n");
    fprintf(stderr, "    -l NovFon : Lista de fonemas introducidos\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    -MS       : Aplicar seseo\n");
    fprintf(stderr, "    -MX       : Transcribir [x] como [ks]\n");
    fprintf(stderr,
            "    -MH       : Transcribir [s] implosiva como [h] siempre\n");
    fprintf(stderr,
            "    -Mh       : Transcribir [s] implosiva como [h] salvo final o enlace\n");
    fprintf(stderr, "                entre palabras\n");
    fprintf(stderr, "    -MK       : Transcribir [sT] como [ks]\n");
    fprintf(stderr, "    -MA       : Transcribir [b], [d] y [g] andinas\n");
    fprintf(stderr,
            "    -My       : Transcribir siempre vocalica la conjuncion /y/\n");
    fprintf(stderr,
            "    -MEb      : Eliminar [b] entre vocales y al final de palabra\n");
    fprintf(stderr,
            "    -MEd      : Eliminar [d] entre vocales y al final de palabra\n");
    fprintf(stderr,
            "    -MEg      : Eliminar [g] entre vocales y al final de palabra\n");
    fprintf(stderr, "    -MN       : Velarizar las [n] implosivas\n");
    fprintf(stderr,
            "    -MM       : Velarizar todas las nasales implosivas\n");
    fprintf(stderr, "    -M~       : Nasalizar vocales\n");
    fprintf(stderr,
            "    -MP       : Representar [pbdtkgx] implosivas internas como [G]\n");
    fprintf(stderr, "    -MR       : Transcribir [r] implosiva como [R]\n");
    fprintf(stderr,
            "    -M@       : Transcribir [l] y [r] tras [pbd...] como [@l] [@r]\n");
    fprintf(stderr,
            "    -M:       : Marcar consonantes implosivas con [:]\n");
    fprintf(stderr, "    -M_       : Marcar vocales postonicas con [_]\n");
    fprintf(stderr,
            "    -M.       : Marcar con . los fonemas inicial y final de palabra\n");
    fprintf(stderr,
            "    -MC       : Separar las oclusiones (p: pcl p   t: tcl t ...)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    -e FicErr : Fichero de errores\n");
    fprintf(stderr,
            "\nTxtIn = \"-\" implica lectura de entrada estandar (teclado). Por defecto,\n");
    fprintf(stderr,
            "(FonOut no indicado) se escribe en salida estandar (pantalla)\n");
    fprintf(stderr,
            "\nPor defecto se realiza solo la transcripcion fonetica (ext. .fon)\n");
    fprintf(stderr,
            "\nLa transcripcion fonetica siempre conserva los silencios. Las otras,\n");
    fprintf(stderr, "si no se indica la opcion -S, no los conservan.\n");
    fprintf(stderr,
            "\nLos diccionarios de excepciones ortograficas y substitucion de grupos\n");
    fprintf(stderr,
            "foneticos permiten el uso de * al principio y/o final de la palabra\n");
    fprintf(stderr,
            "\nPara encadenar mas de un diccionario, usar: FicDic1+FicDic2...\n");

    return;
}
