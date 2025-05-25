import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

# --- CONFIGURAÇÕES ---
PORTA_SERIAL = 'COM8'       # Altere para a porta certa no seu PC (ex: 'COM3' no Windows ou '/dev/ttyUSB0' no Linux)
BAUDRATE = 115200
TAMANHO_JANELA = 100         # Quantos pontos mostrar no gráfico

# --- INICIALIZA SERIAL ---
ser = serial.Serial(PORTA_SERIAL, BAUDRATE, timeout=1)

# --- BUFFER CIRCULAR PARA OS DADOS ---
dados = deque([0]*TAMANHO_JANELA, maxlen=TAMANHO_JANELA)

# --- FUNÇÃO DE ATUALIZAÇÃO DO GRÁFICO ---
def atualizar(frame):
    global dados
    try:
        linha = ser.readline().decode().strip()
        if linha.isdigit():
            valor = int(linha)
            dados.append(valor)
            linha_texto.set_text(f'ADC: {valor}')
    except:
        pass
    
    linha_grafico.set_data(range(len(dados)), dados)
    return linha_grafico, linha_texto

# --- CONFIGURAÇÃO DO PLOT ---
fig, ax = plt.subplots()
linha_grafico, = ax.plot([], [], lw=2)
linha_texto = ax.text(0.8, 0.9, '', transform=ax.transAxes)
ax.set_ylim(0, 4095)
ax.set_xlim(0, TAMANHO_JANELA)
ax.set_title("Leitura do Potenciômetro (ADC)")
ax.set_xlabel("Amostras")
ax.set_ylabel("Valor ADC (0–4095)")
ax.grid(True)

# --- ANIMAÇÃO ---
ani = animation.FuncAnimation(fig, atualizar, interval=10)

# --- MOSTRAR PLOT ---
plt.tight_layout()
plt.show()
