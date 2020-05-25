package br.com.boilercontrol;

import java.awt.Color;

public enum Colors {
    ALL_OFF(Color.WHITE),
    HEATER_ON(Color.RED),
    PUMP_ON(Color.GREEN);
	 
    public Color cor;
    Colors(Color valorCor) {
        cor = valorCor;
    }
}
