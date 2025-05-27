import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

# --- CONFIGURAÇÕES ---
PORTA_SERIAL = 'COM8'       # Altere conforme necessário
BAUDRATE = 115200
TAMANHO_JANELA = 100         # Amostras visíveis

# --- INICIALIZA SERIAL ---
ser = serial.Serial(PORTA_SERIAL, BAUDRATE, timeout=0.05)

# --- BUFFER DE DADOS ---
dados = deque([0]*TAMANHO_JANELA, maxlen=TAMANHO_JANELA)
estado_cooler = "Desconhecido"

# --- FUNÇÃO PARA DETERMINAR O ESTADO ---
def determinar_estado(adc):
    if adc <= 1023:
        return "DESLIGADO"
    elif adc <= 2047:
        return "FRACO"
    elif adc <= 3071:
        return "MEDIO"
    else:
        return "FORTE"

# --- FUNÇÃO DE ATUALIZAÇÃO ---
def atualizar(frame):
    global dados, estado_cooler
    while ser.in_waiting:
        try:
            linha = ser.readline().decode().strip()
            if linha.isdigit():
                valor = int(linha)
                dados.append(valor)
                estado_cooler = determinar_estado(valor)
        except:
            continue

    if dados:
        linha_texto.set_text(f'ADC: {dados[-1]} | Cooler: {estado_cooler}')
        linha_grafico.set_data(range(len(dados)), dados)

    return linha_grafico, linha_texto

# --- CONFIGURAÇÃO DO GRÁFICO ---
fig, ax = plt.subplots()
linha_grafico, = ax.plot([], [], lw=2)
linha_texto = ax.text(0.02, 0.95, '', transform=ax.transAxes)

ax.set_ylim(0, 4095)
ax.set_xlim(0, TAMANHO_JANELA)
ax.set_title("Leitura do ADC + Status do COOLER")
ax.set_xlabel("Amostras")
ax.set_ylabel("Valor ADC (0–4095)")
ax.grid(True)

# --- ANIMAÇÃO ---
ani = animation.FuncAnimation(fig, atualizar, interval=20)

# --- MOSTRAR GRÁFICO ---
plt.tight_layout()
plt.show()
