import json
import pandas as pd
import requests
import sys


ucenici = pd.read_csv('ucenici.csv', sep = ',')
razredi = pd.read_csv('razredi.csv', sep = ',')
"""
lok_row = razredi[razredi['id'] == razred_id]
lok_row = ucenici[ucenici['id'] == razred_id]
IME = float(sen_row['ime'].values[2])
PREZIME = float(sen_row['prezime'].values[2])
RAZRED = float(sen_row['razred'].values[2])
SMJER = float(sen_row['smjer'].values[2])
"""
def kreiraj_list(podaci):
    lista = {
    "Autor" : "Lean Brčić",
    "podaci": {
        "ime": str(podaci.iloc[1]),
        "prezime": str(podaci.iloc[2]),
        "Razred": str(podaci.iloc[5]),
        "Smjer": str(podaci.iloc[6])
        }
    },
    return lista

def spremi_lokalno(podaci,naziv_datoteke):

    try:
        with open(naziv_datoteke, 'w') as f:
            json.dump(podaci,f, indent=4)
