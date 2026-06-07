#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <termios.h>
 
#define MAX_LINEA 200
#define MAX_COMANDOS 10
#define MAX_ARGUMENTOS 32
#define MAX_HISTORIAL 100
 
#define COLOR_PROMPT "\033[1;32m"
#define RESET_COLOR  "\033[0m"
 
typedef struct {
     char *argumentos[MAX_ARGUMENTOS]; 
     char *archivo_entrada;        
     char *archivo_salida;       
     int anexar_salida;     
     char *delimitador_heredoc;
} comando_t;

/*Estructura para manejar el historial */
typedef struct {
    char *comandos[MAX_HISTORIAL]; //Arreglo de comandos
    int contador; //Total de comandos guardados 
    int indice;                 
} historial_t;

/* Variables globales para el historial */
static historial_t historial = { .contador = 0, .indice = 0 };

/* Maneja la señal Ctrl+C (SIGINT) */
static void manejar_sigint(int senial) {
    (void)senial;
    write(STDOUT_FILENO, "\n", 1);
}

/* Muestra el prompt */
static void imprimir_prompt(void) {
    char directorio_actual[PATH_MAX];
    char *usuario = getenv("USER");
    
    if (usuario == NULL) {
        struct passwd *pw = getpwuid(getuid());
        if (pw != NULL) {
            usuario = pw->pw_name;
        } else {
            usuario = "usuario";
        }
    }
    
    if (getcwd(directorio_actual, sizeof(directorio_actual))) {
        // Directorio obtenido correctamente */
    } else {
        strcpy(directorio_actual, "?");
    }
    
    printf(COLOR_PROMPT "%s@%s$ " RESET_COLOR, usuario, directorio_actual);
    fflush(stdout);
}

/* Guarda un comando en el historial */
static void guardar_en_historial(const char *comando){
    //No guarda comandos vacios o repetidos (verificando con el anterior)
    if (strlen(comando) == 0) return;
    if (historial.contador > 0 && strcmp(comando, historial.comandos[historial.contador - 1]) == 0){
        return;
    }
    
    /* Si el historial está lleno, libera el más antiguo */
    if (historial.contador >= MAX_HISTORIAL) {
        free(historial.comandos[0]);
        /* Desplaza los comandos hacia la izquierda */
        for (int i = 0; i < historial.contador - 1; i++) {
            historial.comandos[i] = historial.comandos[i + 1];
        }
        historial.contador--;
    }
    
    /* Guarda el nuevo comando */
    historial.comandos[historial.contador] = strdup(comando);
    if (historial.comandos[historial.contador]){
        historial.contador++;
    }
    historial.indice = historial.contador; /* Posiciona en el final */
}

//Restaura la configuracion de la terminal original
static void restaurar_configuracion_terminal(struct termios *config_original) {
    tcsetattr(STDIN_FILENO, TCSANOW, config_original);
}

/* Configura el terminal para lectura de caracteres individuales */
static void configurar_terminal_para_lectura(struct termios *config_original) {
    struct termios nueva_config;
    tcgetattr(STDIN_FILENO, config_original);
    nueva_config = *config_original;
    nueva_config.c_lflag &= ~(ICANON | ECHO); /* Desactiva modo canónico y eco */
    nueva_config.c_cc[VMIN] = 1;
    nueva_config.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &nueva_config); //Aplicamos esa nueva config
}

/* Lee una linea con soporte de historial, navegación y borrado */
static char *leer_linea_con_historial(void)
{
    char linea[MAX_LINEA + 1] = {0};    
    int  posicion= 0;           /* Cursor dentro del buffer   */
    int  longitud_linea = 0;    /* Longitud actual de la línea*/
    struct termios config_original;     /* Lo usaremos para restaurar terminal */

    //Desactivamos el modo canonico y el echo para poder ir viendo que ingresa el usuario en tiempo real
    configurar_terminal_para_lectura(&config_original);
    while (1)
    {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {
            restaurar_configuracion_terminal(&config_original);
            return NULL;           /* EOF o error */
        }

        // Historial, desplazamiento inline y borrado
        if (c == '\033'){              /*Detectamos si el usuario presiono una tecla especial, como las flechas, backspace y borrado*/
            char secuencia[2];
            if (read(STDIN_FILENO, &secuencia[0], 1) <= 0) { 
                continue; 
            }
            if (read(STDIN_FILENO, &secuencia[1], 1) <= 0) { 
                continue; 
            }

            /* Flecha para arriba */
            if (secuencia[0] == '[' && secuencia[1] == 'A') {
                if (historial.indice > 0) {
                    historial.indice--;
                    const char *comando_anterior = historial.comandos[historial.indice];

                    strncpy(linea, comando_anterior, MAX_LINEA);
                    linea[MAX_LINEA] = '\0';
                    longitud_linea=(int)strlen(linea);
                    posicion = longitud_linea;

                    /* Borra linea completa y vuelve a dibujar prompt con su respectivo  comando */
                    printf("\r\033[K");
                    imprimir_prompt();
                    printf("%s", linea);
                    fflush(stdout);
                }
                continue;
            }

            /* Flecha para abajo */
            if (secuencia[0] == '[' && secuencia[1] == 'B') {
                if (historial.indice < historial.contador - 1) {
                    historial.indice++;
                    const char *comando_siguiente = historial.comandos[historial.indice];

                    strncpy(linea, comando_siguiente, MAX_LINEA);
                    linea[MAX_LINEA] ='\0';

                    longitud_linea = (int)strlen(linea);
                    posicion = longitud_linea;
                }
                else {
                    // Ya llegamos al final del historial
                    historial.indice = historial.contador;
                    linea[0] = '\0';
                    longitud_linea = 0;
                    posicion= 0;
                }
                printf("\r\033[K");
                imprimir_prompt();
                printf("%s", linea);
                fflush(stdout);
                continue;
            }

            /* Flecha izquierda */
            if (secuencia[0] == '[' && secuencia[1] == 'D') {
                if (posicion > 0) {
                    printf("\033[D"); /* Mover cursor a la izquierda */
                    fflush(stdout);
                    posicion--;
                }
                continue;
            }

            /* Flecha derecha */
            if (secuencia[0] == '[' && secuencia[1] == 'C') {
                if (posicion < longitud_linea) {
                    printf("\033[C"); /* Mover cursor a la derecha */
                    fflush(stdout);
                    posicion++;
                }
                continue;
            }

            /* Tecla Suprimir para borrar caracteres de enfrente */
            if (secuencia[0] == '[' && secuencia[1] == '3') {
                char caracter_extra;
                if (read(STDIN_FILENO, &caracter_extra, 1) <= 0) {
                    continue;
                }

                if (caracter_extra == '~') {
                    if (posicion < longitud_linea) {
                        /* Desplazar caracteres hacia la izquierda */
                        for (int i = posicion; i < longitud_linea; i++) {
                            linea[i] = linea[i + 1];
                        }
                        longitud_linea--;
                        linea[longitud_linea] = '\0';
                        
                        /* Redibujar desde posición actual */
                        printf("%s ", &linea[posicion]); /* Texto + espacio extra */
                        int caracteres_restantes = longitud_linea - posicion + 1;
                        if (caracteres_restantes > 0) {
                            printf("\033[%dD", caracteres_restantes); /* Reposicionar cursor */
                        }
                        fflush(stdout);
                    }
                }
                continue;
            }
        }

        //Enter
        if (c == '\n') {
            break;
        }

        /* Tecla para eliminar (Backspace) */
        if (c == 127 || c == '\b') {
            if (posicion > 0) {
                for (int i = posicion - 1; i < longitud_linea; i++) {
                    linea[i] = linea[i + 1];
                }
                posicion--;
                longitud_linea--;
                linea[longitud_linea] = '\0';

                /* Actualizar pantalla */
                printf("\b");
                printf("%s ", &linea[posicion]);           /* resto + espacio */
                int restantes = longitud_linea - posicion + 1;
                if (restantes > 0) {
                    printf("\033[%dD", restantes); //Despues de borrar, movemos el cursos hacia la izquierda
                }
                fflush(stdout); //La salida se muestra automaticamente
            }
            continue;
        }

        /* Impresion de caracteres*/
        if (c >= 32 && c < 127 && longitud_linea < MAX_LINEA) {

            /* Hacer hueco si estamos en medio de la línea */
            if (posicion < longitud_linea) {
                for (int i = longitud_linea; i > posicion; i--) {
                    linea[i] = linea[i - 1]; //Hacemos espacio hacia la derecha
                }
            }

            linea[posicion] = c;
            posicion++;
            longitud_linea++;
            linea[longitud_linea] = '\0';
            putchar(c); //Mostramos el carcater

            /* Si hay texto despues, redibujarlo y regresar el cursor   */
            if (posicion < longitud_linea) {
                printf("%s", &linea[posicion]);
                int restantes = longitud_linea - posicion;
                if (restantes > 0) {
                    printf("\033[%dD", restantes); //Deplazamos a la derecha
                }
            }
            fflush(stdout);
            continue; //Volvemos a ejecutar el bucle para ver que mas mete el usuario
        }
    }

    /* Restaurar terminal y cerrar linea */
    restaurar_configuracion_terminal(&config_original);
    printf("\n");

    /* Devolver copia dinámica de la linea */
    return strdup(linea);
}


/* Analiza la linea de entrada en tokens considerando comillas */
static int analizar_linea(char *linea, comando_t comandos[], int *num_comandos) {
    int indice_comando=0;
    int indice_argumento=0;
    char *tokens[MAX_LINEA];  /* Arreglo para almacenar tokens */
    int num_tokens=0;  /* Contador de tokens */
    char *puntero=linea;    /* Puntero para recorrer la línea */

    /* Dividir la línea en tokens considerando comillas */
    while(*puntero){
        /* Saltar espacios y tabs */
        while (*puntero == ' ' || *puntero == '\t') {
            puntero++;
        }
        
        /* Si llegamos al final, salir */
        if (*puntero == '\0') {
            break;
        }

        /* Manejar comillas simples o dobles */
        if (*puntero == '"' || *puntero == '\'') {
            char comilla = *puntero;
            puntero++;  /* Avanzar después de la comilla */
            if (num_tokens >= MAX_LINEA) {
                fprintf(stderr, "Demasiados tokens\n");
                return -1;
            }
            tokens[num_tokens] = puntero;  /* Inicio del token */
            num_tokens++;

            /* Buscar comilla de cierre */
            while (*puntero && *puntero != comilla) {
                puntero++;
            }

            if (*puntero == comilla) {
                *puntero = '\0';  /* Terminar el token */
                puntero++;
            } else {
                fprintf(stderr, "Error: comilla sin cerrar\n");
                return -1;
            }
        } 
        /* Manejar tokens normales */
        else {
            if (num_tokens >= MAX_LINEA) {
                fprintf(stderr, "Demasiados tokens\n");
                return -1;
            }
            tokens[num_tokens] = puntero; //Guardamos la direccion actual del apuntador, indicando que a partir de ahi empieza un nuevo argumento
            num_tokens++;
            
            /* Avanzar hasta el próximo espacio o tab */
            while (*puntero && *puntero != ' ' && *puntero != '\t') {
                puntero++;
            }
            
            if (*puntero){ //Si el apuntador llego al final de la linea entonces ahi termina el token
                *puntero = '\0';
                puntero++;
            }
        }
    }

    //Inicializamos la estructura de comandos para evitar que haya basura
    *num_comandos = 0;
    memset(comandos, 0, sizeof(comando_t) * MAX_COMANDOS);

    if (num_tokens == 0) {
        return 0;
    }

    /* Procesar cada token para construir comandos */
    for (int t = 0; t < num_tokens; t++) {
        char *token_actual = tokens[t];

        /* Manejar pipe entre comandos */
        if (strcmp(token_actual, "|") == 0) {
            if (indice_argumento == 0) {
                fprintf(stderr, "Error: comando vacio cerca de pipe\n");
                return -1;
            }
            comandos[indice_comando].argumentos[indice_argumento] = NULL; //El comando anterior termina
            indice_comando++; //Preparamos el siguiente comando
            indice_argumento = 0; //Se establece en cero para indicar que el siguiente comando aun no tiene ningun argumento guardado
            if (indice_comando >= MAX_COMANDOS) {
                fprintf(stderr, "Demasiados comandos\n");
                return -1;
            }
        } 
        /* Manejar redirección de entrada */
        else if (strcmp(token_actual, "<") == 0) {
            t++; 
            if (t >= num_tokens) {
                fprintf(stderr, "Error: falta archivo para <\n");
                return -1;
            }
            comandos[indice_comando].archivo_entrada = tokens[t];
        } 
        /* Manejar redireccion de salida (append) */
        else if (strcmp(token_actual, ">>") == 0) {
            t++; 
            if (t >= num_tokens) {
                fprintf(stderr, "Error: falta archivo para >>\n");
                return -1;
            }
            comandos[indice_comando].archivo_salida = tokens[t];
            comandos[indice_comando].anexar_salida = 1;
        } 
        /* Manejar redireccion de salida (sobreescribir el archivo) */
        else if (strcmp(token_actual, ">") == 0) {
            t++;
            if (t >= num_tokens) {
                fprintf(stderr, "Error: falta archivo para >\n");
                return -1;
            }
            comandos[indice_comando].archivo_salida = tokens[t];
            comandos[indice_comando].anexar_salida = 0;
        } 
        /* Manejar operador heredoc */
        else if (strcmp(token_actual, "<<") == 0) {
            t++;
            if (t >= num_tokens) {
                fprintf(stderr, "Error: falta delimitador para <<\n");
                return -1;
            }
            comandos[indice_comando].delimitador_heredoc = tokens[t];
        } 
        /* Manejar argumentos normales */
        else{
            comandos[indice_comando].argumentos[indice_argumento] = token_actual;
            indice_argumento++;
            if (indice_argumento >= MAX_ARGUMENTOS-1){
                fprintf(stderr, "Demasiados argumentos\n");
                return -1;
            }
        }
    }

    if (indice_argumento == 0) {
        fprintf(stderr, "Error: falta comando\n");
        return -1;
    }

    /* Terminar el último comando */
    comandos[indice_comando].argumentos[indice_argumento] = NULL;
    *num_comandos = indice_comando + 1;
    return 0;
}

/* Ejecuta un comando con heredoc si está presente */
static void manejar_heredoc(char *delimitador) {
    int tuberia_heredoc[2];
    
    /* Crear tubería para heredoc */
    if (pipe(tuberia_heredoc)) {
        perror("Error al crear tubería para heredoc");
        exit(1);
    }

    pid_t pid_heredoc = fork();
    if (pid_heredoc == 0) {
        /* Proceso hijo: leer entrada para heredoc */
        close(tuberia_heredoc[0]);  /* El proceso hijo no lee por la tuberia */
        char linea_heredoc[MAX_LINEA];
        
        while (1) {
            printf("> ");
            fflush(stdout);
            
            if (fgets(linea_heredoc, sizeof(linea_heredoc), stdin) == NULL) {
                break;  /* Fin de entrada (Ctrl+D) */
            }
            
            /* Eliminar salto de línea */
            size_t longitud = strlen(linea_heredoc);
            if (longitud > 0 && linea_heredoc[longitud-1] == '\n') {
                linea_heredoc[longitud-1] = '\0';
            }
            
            if (strcmp(linea_heredoc, delimitador) == 0) {
                break;
            }
            
            /* Escribir en la tubería */
            write(tuberia_heredoc[1], linea_heredoc, strlen(linea_heredoc));
            write(tuberia_heredoc[1], "\n", 1);
        }
        close(tuberia_heredoc[1]);
        exit(0);
    } 
    else {
        /* Proceso padre: redirigir entrada desde heredoc */
        close(tuberia_heredoc[1]);  // Cerrar extremo de escritura
        waitpid(pid_heredoc, NULL, 0);  
        dup2(tuberia_heredoc[0], STDIN_FILENO);  //No consideramos la entrada desde teclado, consideramos lo que el hijo paso por la tuberia que es el contenido del heredoc
        close(tuberia_heredoc[0]);
    }
}

/* Ejecuta una secuencia de comandos con tuberías y redirecciones */
static int ejecutar_secuencia(comando_t comandos[], int num_comandos) {
    int descriptor_previo = -1;  /* No hay una entrada previa a la tuberia*/
    int estado = 0;              /* Estado de retorno */
    int tuberia[2];              /* Descriptores para nueva tubería */

    for (int i=0; i<num_comandos; i++){
        tuberia[0] = -1;
        tuberia[1] = -1;
        
        /* Crear tubería si no es el último comando */
        if (i < num_comandos - 1) {
            if (pipe(tuberia)) {
                perror("Error al crear tubería");
                return -1;
            }
        }

        pid_t pid = fork();
        if (pid == 0) {
            signal(SIGINT, SIG_DFL);  //Establecemos un comportamiento default para que cuando se ingrese ctrl+c el proceso se termine

            /* Redirigir entrada desde tubería anterior */
            if (descriptor_previo != -1) { //El descriptor de previo guarda el extremo de lectura de la tuberia anterior
                dup2(descriptor_previo, STDIN_FILENO); //Cualquier entrada que se haga vendra del descriptor previo, NO DEL TECLADO
                close(descriptor_previo);
            }
            
            /* Redirigir salida a la siguiente tubería */
            if (i < num_comandos - 1) {
                dup2(tuberia[1], STDOUT_FILENO); //No se imprime en pantalla, se envia a la tuberia
                close(tuberia[0]);
                close(tuberia[1]);
            }
            
            /* Manejar redirección de entrada desde archivo */
            if (comandos[i].archivo_entrada != NULL) {
                int descriptor_archivo = open(comandos[i].archivo_entrada, O_RDONLY);
                if (descriptor_archivo == -1) {
                    perror("Error al abrir archivo de entrada");
                    exit(1);
                }
                dup2(descriptor_archivo, STDIN_FILENO); //Lee solo desde el descriptor
                close(descriptor_archivo);
            }
            
            /* Manejar operador heredoc */
            if (comandos[i].delimitador_heredoc != NULL) {
                manejar_heredoc(comandos[i].delimitador_heredoc);
            }
            
            /* Manejar redirección de salida a archivo */
            if (comandos[i].archivo_salida != NULL) {
                int modo_apertura;
                if (comandos[i].anexar_salida) { //Si anexar salida es 1 se concatena
                    modo_apertura = O_WRONLY | O_CREAT | O_APPEND; /* Modo anexar */ 
                } else {
                    modo_apertura = O_WRONLY | O_CREAT | O_TRUNC;  /* Modo sobreescribir el archivo */
                }
                
                int descriptor_archivo = open(comandos[i].archivo_salida, modo_apertura, 0644);
                if (descriptor_archivo == -1) {
                    perror("Error al abrir archivo de salida");
                    exit(1);
                }
                dup2(descriptor_archivo, STDOUT_FILENO); //Desactiva que no se imprima en pantalla, lo manda al descriptor de archivo
                close(descriptor_archivo);
            }
            
            //Ejecutar el comando
            execvp(comandos[i].argumentos[0], comandos[i].argumentos);
            perror("Error al ejecutar comando");
            exit(1);
        } 
        else if (pid<0){
            /* Error al crear proceso hijo */
            perror("Error al crear proceso");
            return -1;
        } 
        else {
            /* Proceso padre */
            if (descriptor_previo != -1) {
                close(descriptor_previo);
            }
            
            if (i < num_comandos-1){
                close(tuberia[1]);  /* Cerrar extremo de escritura no usado */
                descriptor_previo=tuberia[0];  /* Guardar para próximo comando */
            }
        }
    }

    /* Esperar que todos los procesos hijos terminen */
    while (wait(&estado) > 0) {
        /* Continuamos esperando al hijo a que termine de ejecutar los procesos */
    }
    
    return estado;
}

/* Libera la memoria del historial */
static void liberar_historial(void) {
    for (int i = 0; i < historial.contador; i++){
        free(historial.comandos[i]);
    }
    historial.contador=0;
    historial.indice=0;
}

int main(void) {
    signal(SIGINT, manejar_sigint);  //LO que hace es que SIGINT manda una señal al sistema operativo de que ocurrio una interrupcion como ctrl+c
                                     //y manda a que la trate la funcion
    char *linea=NULL;
    comando_t comandos[MAX_COMANDOS]; 

    /* Registrar manejador para limpiar al salir */
    atexit(liberar_historial);

    while (1) {
        imprimir_prompt();
        
        /* Leer línea con soporte para historial */
        linea = leer_linea_con_historial();
        if (linea == NULL) {
            putchar('\n');
            break; /* Fin de archivo (Ctrl+D) */
        }
        
        guardar_en_historial(linea);
        
        if (strlen(linea) == 0) {
            free(linea);
            continue;
        }

        int num_comandos = 0;
        if (analizar_linea(linea, comandos, &num_comandos) == -1) {
            fprintf(stderr, "Error de sintaxis\n");
            free(linea);
            continue;
        }

        if (num_comandos == 0 || comandos[0].argumentos[0] == NULL) {
            free(linea);
            continue;
        }

        if (strcmp(comandos[0].argumentos[0], "exit") == 0) {
            free(linea);
            break;
        }
        
        if (strcmp(comandos[0].argumentos[0], "cd") == 0) {
            char *directorio;
            if (comandos[0].argumentos[1] != NULL) { //Si el usuario metio como argumento un directorio al cual moverse,
                                                     // simplemente lo almacena en el directorio
                directorio = comandos[0].argumentos[1];
            } else {
                directorio = getenv("HOME");
                if (directorio == NULL) {
                    fprintf(stderr, "Error: variable HOME no definida\n");
                    free(linea);
                    continue;
                }
            }
            
            if (chdir(directorio)) {
                perror("Error en cd");
            }
            free(linea);
            continue;
        }
        ejecutar_secuencia(comandos, num_comandos);
        free(linea);
    }
    
    return 0;
}
