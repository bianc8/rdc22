//160 campioni --> 0.02 s
//posso annullare tutto il pacchetto o meno
//ritardo complessivo < 50ms --> posso basarmi su 2 pacchetti precedenti + mezzo paccheto --> 160 + 160 + 80 campioni

#include<iostream>
#include<fstream>
#include<vector>

using namespace std;

//la decisione riguardo all' azzeramento o meno del pacchetto si basa su questi 3 parametri

const int SOGLIA_CAMPIONE_CONSIDERATO_RUMORE = 9;  //un campione viene considerato rumore e quindi non valido se il suo valore assoluto è minore di questo parametro
const int NUM_CAMPIONI_VALIDI_MINIMI_PER_PACCHETTO = 9; //un pacchetto viene considerato valido e verrà inviato se contiene almeno (tot) campioni validi 
const int NUM_PACCHETTI_DA_INVIARE_PER_EVITARE_IL_CLIPPING = 10; //se un pacchetto viene considerato valido, allora anche i (tot) pacchetti consecutivi verranno inviati per evitare che si taglino le parole finali
//quest' ultimo parametro non si può applicare anche ai pacchetti precedenti perchè il ritardo massimo di 50 ms non lo permette

//scelta progettuale: invio i pacchetti SEMPRE con 50ms di ritardo
//quindi ad ogni metà pacchetto in input, invio il pacchetto pre-precedente (pacchetto molto vecchio), quello che ho iniziato a ricevere 50 ms prima 


int main(int argc, char *argv[]) {
    
    ofstream fout2(argv[3]);
    ofstream fout(argv[2], ios::out | ios::binary);
    ifstream fin(argv[1], ios::in | ios::binary);

    cout<<argv[1]<<endl<<argv[2]<<endl;

    char b;
    vector <char> pacchetto_molto_vecchio(160); //contiene il pacchetto da inviare
    bool molto_vecchio_attivo = false; // indica se il pacchetto molto vecchio dovr� essere inviato o meno
    vector <char> pacchetto_vecchio(160);  //contiene il pacchetto che dovrà essere inviato dopo 20ms
    bool vecchio_attivo = false;// indica se il pacchetto vecchio dovr� essere inviato o meno
    vector <char> pacchetto_corrente(160);

    int anti_clipping = 0; //contatore di pacchetti rimanenti da inviare dopo l' ultimo valido

    int counter = 0; //contatore di campioni validi all' interno di ciascun pacchetto

    int k = 0;
    for(int i = 0; !fin.eof(); i++){
        k++;

        //prelevo la prima met� del pacchetto corrente
        for(int j = 0; j < 80; j++){
            b = fin.get();
            pacchetto_corrente[j] = b;
            if(b >= SOGLIA_CAMPIONE_CONSIDERATO_RUMORE || b <= -SOGLIA_CAMPIONE_CONSIDERATO_RUMORE){
                counter++;
            }
        }

        //fine prima met�, devo inviare il pacchetto molto vecchio (sono passati 50ms)
        //invio sempre il pacchetto di indice i-2

        if(i > 1){
            // se met� del pacchetto corrente � valida oppure se erano validi i precedenti oppure non devo clippare la voce, invio i dati
            if(counter >= SOGLIA_CAMPIONE_CONSIDERATO_RUMORE/2 || molto_vecchio_attivo || anti_clipping > 0){
                for(int j=0; j<160; j++){
                    fout<<pacchetto_molto_vecchio[j];
                }
                cout<<"                  pacc. "<<i - 2<<" inviato "<<endl;
                fout2<<"pacc. "<<i - 2<<" inviato "<<endl;
            }else{                                     // altrimenti annullo il pacchetto
                for(int j=0; j<160; j++){
                    fout<<'\0';
                }
                cout<<"                  pacc. "<<i - 2<<" non inviato "<<endl;
                fout2<<"pacc. "<<i - 2<<" non inviato "<<endl;
            }
        }

        //inizio prelevamento seconda met�
        for(int j = 80; j < 160; j++){
            b = fin.get();
            pacchetto_corrente[j] = b;
            if(b >= SOGLIA_CAMPIONE_CONSIDERATO_RUMORE|| b <= -SOGLIA_CAMPIONE_CONSIDERATO_RUMORE){
                counter++;
            }
        }

        //fine prelevamento pacchetto
        pacchetto_molto_vecchio = pacchetto_vecchio;
        molto_vecchio_attivo = vecchio_attivo;

        pacchetto_vecchio = pacchetto_corrente;

        if(counter >= NUM_CAMPIONI_VALIDI_MINIMI_PER_PACCHETTO){
            molto_vecchio_attivo = true;
            vecchio_attivo = true;
            anti_clipping = NUM_PACCHETTI_DA_INVIARE_PER_EVITARE_IL_CLIPPING;
        }else{
            vecchio_attivo = false;
            anti_clipping --;
        }

        cout<<"pacc. "<<i<<" counter: "<<counter;
        counter = 0;
    }

    //invio degli ultimi 2 pacchetti
    if(molto_vecchio_attivo || anti_clipping > 0){
        for(int j=0; j<160; j++){
            fout<<pacchetto_molto_vecchio[j];
        }
        cout<<"                  pacc. "<<k - 2<<" inviato pen"<<endl;
        fout2<<"pacc. "<<k - 2<<" inviato "<<endl;

        if(!molto_vecchio_attivo)
            anti_clipping --;

    }else{
        for(int j=0; j<160; j++){
            fout<<'\0';
        }
        cout<<"                  pacc. "<<k - 2<<" non inviato pen"<<endl;
        fout2
        <<"pacc. "<<k - 2<<" non inviato "<<endl;
    }
    //invio ultimo
    if(vecchio_attivo || anti_clipping > 0){
        for(int j=0; j<160; j++){
            fout<<pacchetto_vecchio[j];
        }
        cout<<"                  pacc. "<<k - 1<<" inviato ult"<<endl;
        fout2<<"pacc. "<<k - 1<<" inviato "<<endl;            
    }else{
        for(int j=0; j<160; j++){
            fout<<'\0';
        }
        cout<<"                  pacc. "<<k - 1<<" non inviato ult"<<endl;
        fout2<<"pacc. "<<k - 1<<" non inviato "<<endl;
    }

    fout.close();
    fin.close();
    return 0;
}


