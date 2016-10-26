import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.util.Vector;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

public class Gui extends JPanel {

	double phase = 0; // degrees
	double power = 0; // dBm

	public static void main(String[] args) throws Exception {
		JFrame frame = new JFrame("Doppler");
		Gui gui = new Gui();
		frame.add(gui);
		frame.setBounds(50, 350, 800, 500);
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frame.setVisible(true);

		gui.run(frame);
	}

	public Gui() {}

	public void paintComponent(Graphics g) {
		super.paintComponent(g);
		int width = getWidth();
		int height = getHeight();
		int cx = width/2;
		int cy = height/2;
		int r = height / 3;
		g.drawString("Phase = " + phase, 20,20);
		g.drawString("Power = " + power, 20,40);
		g.drawLine(cx, cy, cx + (int)(r * Math.cos(phase / 180 * Math.PI)), cy + (int)(r * Math.sin(phase / 180 * Math.PI)));
	}


    public void run(JFrame frame) throws Exception {
		BufferedReader bufferReader = new BufferedReader(new InputStreamReader(System.in));
		while (true) {
			String[] parts = bufferReader.readLine().split(",");
			phase = phase + 0.3 * ((Double.parseDouble(parts[0]) - phase+720+180)%360-180);
			power = power * 0.7 + 0.3 * Double.parseDouble(parts[1]);
			repaint();
			frame.getToolkit().sync();
			Thread.sleep(10);
		}
	}




}
