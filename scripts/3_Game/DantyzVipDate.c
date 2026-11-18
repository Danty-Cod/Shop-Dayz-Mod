// =====================================================================================
// DATAS DO VIP (calendario real, UTC)
// O "daysRemaining" salvo no JSON era o valor da compra e nunca diminuia.
// Agora os dias restantes sao sempre calculados a partir da data de expiracao.
// =====================================================================================
class DantyzVipDate
{
    // numero do dia (para subtrair datas)
    static int DayNumber(int y, int m, int d)
    {
        if (m <= 2)
        {
            y = y - 1;
            m = m + 12;
        }
        return 365 * y + y / 4 - y / 100 + y / 400 + (153 * (m - 3) + 2) / 5 + d;
    }

    static int DaysInMonth(int m, int y)
    {
        if (m == 2)
        {
            if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))
                return 29;
            return 28;
        }
        if (m == 4 || m == 6 || m == 9 || m == 11)
            return 30;
        return 31;
    }

    // hoje + dias (calendario real)
    static void AddDays(int days, out int y, out int m, out int d)
    {
        GetYearMonthDayUTC(y, m, d);
        for (int i = 0; i < days; i++)
        {
            d++;
            if (d > DaysInMonth(m, y))
            {
                d = 1;
                m++;
                if (m > 12)
                {
                    m = 1;
                    y++;
                }
            }
        }
    }

    // le "YYYY-MM-DD..." (ISO) ou "DD/MM/YYYY"
    static bool Parse(string date, out int y, out int m, out int d)
    {
        y = 0;
        m = 0;
        d = 0;
        if (date == "")
            return false;

        array<string> parts = new array<string>();
        if (date.IndexOf("/") > 0)
        {
            date.Split("/", parts);
            if (parts.Count() != 3)
                return false;
            d = parts[0].ToInt();
            m = parts[1].ToInt();
            y = parts[2].ToInt();
        }
        else
        {
            string datePart = date;
            int tIdx = date.IndexOf("T");
            if (tIdx > 0)
                datePart = date.Substring(0, tIdx);
            datePart.Split("-", parts);
            if (parts.Count() < 3)
                return false;
            y = parts[0].ToInt();
            m = parts[1].ToInt();
            d = parts[2].ToInt();
        }
        return y > 0 && m >= 1 && m <= 12 && d >= 1 && d <= 31;
    }

    // dias inteiros ate a expiracao: 0 = vence hoje (ainda vale ate 23:59), -1 = vencido/invalido
    static int DaysLeft(string expirationDate)
    {
        int ey, em, ed;
        if (!Parse(expirationDate, ey, em, ed))
            return -1;

        int y, m, d;
        GetYearMonthDayUTC(y, m, d);
        int left = DayNumber(ey, em, ed) - DayNumber(y, m, d);
        if (left < 0)
            return -1;
        return left;
    }

    // o que aparece no texto "VIP: X dias" (ultimo dia conta como 1)
    static int DisplayDays(string expirationDate)
    {
        int left = DaysLeft(expirationDate);
        if (left < 0)
            return 0;
        if (left == 0)
            return 1;
        return left;
    }

    static string BuildIso(int y, int m, int d, bool endOfDay)
    {
        string s = y.ToString() + "-";
        if (m < 10) s += "0";
        s += m.ToString() + "-";
        if (d < 10) s += "0";
        s += d.ToString();
        if (endOfDay) s += "T23:59:59Z";
        else          s += "T00:00:00Z";
        return s;
    }
}
