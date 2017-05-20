#!/bin/bash

function init {

    cell=( [00]=' ' [01]=' ' [02]=' ' )
    cell+=( [10]=' ' [11]=' ' [12]=' ' )
    cell+=( [20]=' ' [21]=' ' [22]=' ' )

    turn='X'
    movesNum=0

    AVAILABLE_COORDS_REGEXP="[123][123]"

    chooseSide
    redrawScreen
}

function chooseSide {
    nc localhost 9999
    if [[ $? -ne 0 ]]
        then
            mark='X'
            echo 'Ожидаем подключения второй стороны.'
            echo '' | nc -l -p 9999
        else
            mark='O'
    fi
}

function redrawScreen {

    clear

    i=0
    while [[ ${i} -lt 3 ]]
    do
        j=0
        echo "-------"
        while [[ ${j} -lt 3 ]]; do
            echo -n "|${cell[${i}${j}]}"
            j=$(( j + 1 ))
        done
        echo "|"
        i=$(( i + 1 ))
    done
    echo "-------"

    echo "Вы играете за ${mark}"
}

function readCoords {

    while true
    do
        read coords

        if ! [[ ${#coords} -eq 2 && ${coords} == ${AVAILABLE_COORDS_REGEXP} ]]
        then
            echo 'Неправильные координаты. Координаты - две цифры от 1 до 3'
            continue
        fi

        coords=$(( coords - 11 ))
        if [[ ${cell[${coords}]} != ' ' ]]
        then
            echo 'Клетка уже занята. Попробуйте другую.'
            continue
        fi

        break
    done
}

function makeMove {

    if [[ ${turn} == ${mark} ]]
    then
        yourTurn
    else
        opponentTurn
    fi
    cell[${coords}]=${turn}

    # Меняем того, кто сейчас ходит
    if [[ ${turn} == 'X' ]]
        then
            turn='O'
        else
            turn='X'
    fi

    movesNum=$(( movesNum + 1 ))
}

function yourTurn {
    echo 'Ваш ход. Введите строчку и столбик без пробела.'
    readCoords
    echo "${coords}" | nc -l -p 9999
}

function opponentTurn {
    echo 'Ход противника.'
    coords=`nc localhost 9999`
    while ! [[ ${coords} ]]; do
        coords=`nc localhost 9999`
    done
    echo ${coords}
}

# Проверяем условия конца игры и выходим, если все
function finish {
    # Смотрим, есть ли 3 в ряд
    i=$(( coords / 10 ))
    i1=$(( ($i + 1) % 3))
    i2=$(( ($i + 2) % 3))
    j=$(( coords % 10 ))
    j1=$(( ($j + 1) % 3))
    j2=$(( ($j + 2) % 3))
    curMark=${cell[${coords}]}

    message=''
    if [[ ${cell[${i}${j1}]} == ${curMark} && ${cell[${i}${j2}]} == ${curMark} || \
          ${cell[${i1}${j}]} == ${curMark} && ${cell[${i2}${j}]} == ${curMark} || \
          ${i} == ${j} && ${cell[${i1}${j1}]} == ${curMark} && ${cell[${i2}${j2}]} == ${curMark} || \
          $(( ${i} + ${j} )) == 2 && ${cell[${i1}${j2}]} == ${curMark} && ${cell[${i2}${j1}]} == ${curMark} ]]
    then
        if [[ ${curMark} == ${mark} ]]
        then
            message='Вы победили.'
        else
            message='Вы проиграли.'
        fi
    elif [[ ${movesNum} == 9 ]]
    then
        message='Ничья.'
    fi

    if [[ ${message} != '' ]]
        then
            echo ${message}
            exit
    fi
}

init
while true; do
    makeMove
    redrawScreen
    finish
done
