#! /bin/sh

error() {
    echo $*
    exit 2
}

# Before doing anything check for root rights.
if [ `id -u` -ne  0 ]; then
    echo "You must be root to run this script."
    exit 1
fi

make || error "Compile error."
himage -e CnC > /dev/null 2>&1 || error "Is IMUNES simulation started? (imunes topo_lab1.imn) Try: Experiment->Execute"

# terminiraj sve postojece programi koji mozda vec rade 
himage CnC killall -q -9 CandC.py
himage BOT1 killall -q -9 bot
himage UDP_server killall -q -9 UDP_server

# kopiraj sve potrebne datoteke na cvorove
hcp CandC.py CnC:/
hcp bot BOT1:/
hcp UDP_server UDP_server:/

# postavi IP adresu i port UPD servera u CandC.py programu na dobre vrijednosti
himage CnC sed -i.bak 's/UDP_ip = "\([^"]*\)"/UDP_ip = "10.0.0.20"/' /CandC.py
himage CnC sed -i.bak 's/UDP_port = "\([^"]*\)"/UDP_port = "1234"/' /CandC.py

# pokreni sve programe u posebnim terminal prozorima
xfce4-terminal --geometry 70x25+0+0 -T 'CandC.py' -e "bash -c 'himage CnC python3 /CandC.py';bash" &
xfce4-terminal --geometry 70x10-0+0 -T 'UDP_server' -e "bash -c 'himage UDP_server /UDP_server -p \"ovo je payload\"';bash" &
sleep 3
xfce4-terminal --geometry 70x10+0-0 -T 'bot' -e "bash -c 'himage BOT1 /bot 10.0.0.10 5555';bash" &
xfce4-terminal --geometry 70x10-0-400 -T 'zrtva1' -e "bash -c 'himage zrtva1 nc -kul -p 1111';bash" &
xfce4-terminal --geometry 70x10-0-200 -T 'zrtva2' -e "bash -c 'himage zrtva2 nc -kul -p 2222';bash" &
xfce4-terminal --geometry 70x10-0-0 -T 'zrtva3' -e "bash -c 'himage zrtva3 nc -kul -p 3333';bash" &

