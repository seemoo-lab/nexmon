package de.tu_darmstadt.seemoo.nexmon.stations;

/**
 * Created by fabian on 9/8/16.
 */
public class AttackInfoString {
    public static final int ATTACK_UPDATE_ERROR = 1;
    public static final int ATTACK_UPDATE_SUCCESS = 2;
    public static final int ATTACK_UPDATE_RUNNING = 3;


    public int messageType;
    public String message;

    public AttackInfoString() {}

    public AttackInfoString(int messageType, String message) {
        this.message = message;
        this.messageType = messageType;
    }
}
