import java.util.Scanner;

public class StdStd {

	public static void main(String[] args) {
		Scanner in = new Scanner(System.in);
		int sum = 0;
		while (in.hasNextInt())
			sum += in.nextInt();
		System.out.println(sum);
		in.close();
	}

}
