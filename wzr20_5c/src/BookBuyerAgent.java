/*
 *  Klasa agenta kupującego książki
 *
 *  Argumenty projektu (NETBEANS: project properties/run/arguments):
 *  -agents seller1:BookSellerAgent();seller2:BookSellerAgent();buyer1:BookBuyerAgent(Zamek) -gui
 */

import jade.core.Agent;
import jade.core.AID;
import jade.core.behaviours.*;
import jade.lang.acl.*;

import java.util.*;

// Przykładowa klasa zachowania:
class MyOwnBehaviour extends Behaviour {
    protected MyOwnBehaviour() {
    }

    public void action() {
    }

    public boolean done() {
        return false;
    }
}

public class BookBuyerAgent extends Agent {

    private String targetBookTitle;    // tytuł kupowanej książki przekazywany poprzez argument wejściowy
    // lista znanych agentów sprzedających książki (w przypadku użycia żółtej księgi - usługi katalogowej, sprzedawcy
    // mogą być dołączani do listy dynamicznie!
    private AID[] sellerAgents = {
            new AID("seller1", AID.ISLOCALNAME),
            new AID("seller2", AID.ISLOCALNAME)};

    // Inicjalizacja klasy agenta:
    protected void setup() {

        //doWait(4000);   // Oczekiwanie na uruchomienie agentów sprzedających

        System.out.println("Witam! Agent-kupiec " + getAID().getName() + " (wersja c lato, 2019/20) jest gotów!");

        Object[] args = getArguments();  // lista argumentów wejściowych (tytuł książki)

        if (args != null && args.length > 0)   // jeśli podano tytuł książki
        {
            targetBookTitle = (String) args[0];
            System.out.println("Zamierzam kupić książkę zatytułowaną " + targetBookTitle);

            addBehaviour(new RequestPerformer());  // dodanie głównej klasy zachowań - kod znajduje się poniżej

        } else {
            // Jeśli nie przekazano poprzez argument tytułu książki, agent kończy działanie:
            System.out.println("Należy podać tytuł książki w argumentach wejściowych kupca!");
            doDelete();
        }
    }

    // Metoda realizująca zakończenie pracy agenta:
    protected void takeDown() {
        System.out.println("Agent-kupiec " + getAID().getName() + " kończy istnienie.");
    }

    /**
     * Inner class RequestPerformer.
     * This is the behaviour used by Book-buyer agents to request seller
     * agents the target book.
     */
    private class RequestPerformer extends Behaviour {

        private AID bestSeller;     // agent sprzedający z najkorzystniejszą ofertą
        private int bestPrice;      // najlepsza cena
        private int repliesCnt = 0; // liczba odpowiedzi od agentów
        private MessageTemplate mt; // szablon odpowiedzi
        private int step = 0;       // krok
        private double lastPrice = 0;
        private int counter = 0;
        HashMap<String, Integer> oferty = new HashMap<>();

        public void action() {
            switch (step) {
                case 0:      // wysłanie oferty kupna
                    System.out.print(" Oferta kupna (CFP) jest wysyłana do: ");
                    for (int i = 0; i < sellerAgents.length; ++i) {
                        System.out.print(sellerAgents[i] + " ");
                    }
                    System.out.println();

                    // Tworzenie wiadomości CFP do wszystkich sprzedawców:
                    ACLMessage cfp = new ACLMessage(ACLMessage.CFP);
                    for (int i = 0; i < sellerAgents.length; ++i) {
                        cfp.addReceiver(sellerAgents[i]);          // dodanie adresate
                    }
                    cfp.setContent(targetBookTitle);             // wpisanie zawartości - tytułu książki
                    cfp.setConversationId("book-trade");         // wpisanie specjalnego identyfikatora korespondencji
                    cfp.setReplyWith("cfp" + System.currentTimeMillis()); // dodatkowa unikatowa wartość, żeby w razie odpowiedzi zidentyfikować adresatów
                    myAgent.send(cfp);                           // wysłanie wiadomości

                    // Utworzenie szablonu do odbioru ofert sprzedaży tylko od wskazanych sprzedawców:
                    mt = MessageTemplate.and(MessageTemplate.MatchConversationId("book-trade"),
                            MessageTemplate.MatchInReplyTo(cfp.getReplyWith()));
                    step = 1;     // przejście do kolejnego kroku
                    break;
                case 1:      // odbiór ofert sprzedaży/odmowy od agentów-sprzedawców
                    ACLMessage reply = myAgent.receive(mt);      // odbiór odpowiedzi
                    if (reply != null) {
                        if (reply.getPerformative() == ACLMessage.PROPOSE)   // jeśli wiadomość jest typu PROPOSE
                        {
                            int price = Integer.parseInt(reply.getContent());  // cena książki
                            if (bestSeller == null || price < bestPrice)       // jeśli jest to najlepsza oferta
                            {
                                bestPrice = price;
                                bestSeller = reply.getSender();
                            }
                        }
                        repliesCnt++;                                        // liczba ofert
                        if (repliesCnt >= sellerAgents.length)               // jeśli liczba ofert co najmniej liczbie sprzedawców
                        {
                            step = 5;
                        }
                    } else {
                        block();
                    }
                    break;
                case 2:      // wysłanie zamówienia do sprzedawcy, który złożył najlepszą ofertę
                    ACLMessage order = new ACLMessage(ACLMessage.ACCEPT_PROPOSAL);
                    order.addReceiver(bestSeller);
                    order.setContent(targetBookTitle);
                    order.setConversationId("book-trade");
                    order.setReplyWith("order" + System.currentTimeMillis());
                    myAgent.send(order);
                    mt = MessageTemplate.and(MessageTemplate.MatchConversationId("book-trade"),
                            MessageTemplate.MatchInReplyTo(order.getReplyWith()));
                    step = 3;
                    break;
                case 3:      // odbiór odpowiedzi na zamównienie
                    reply = myAgent.receive(mt);
                    if (reply != null) {
                        if (reply.getPerformative() == ACLMessage.INFORM) {
                            System.out.println("Tytuł " + targetBookTitle + " został zamówiony.");
                            System.out.println("Cena = " + bestPrice);
                            myAgent.doDelete();
                        }
                        step = 4;
                    } else {
                        block();
                    }
                    break;
                case 5:
                    System.out.println("Wysylam offerte do najlepszego sprzedawcy:" + bestSeller.toString());
                    lastPrice = 0.6 * bestPrice;
                    ACLMessage newPriceReply = new ACLMessage(ACLMessage.REQUEST);
                    newPriceReply.setConversationId("Bidding");
                    newPriceReply.addReceiver(bestSeller);
                    newPriceReply.setContent(String.valueOf(lastPrice));
                    myAgent.send(newPriceReply);
                    System.out.println("Proponowana cena: " + lastPrice);
                    step = 6;
                    break;
                case 6:
                    MessageTemplate mt2 = MessageTemplate.MatchPerformative(21);
                    ACLMessage replyOffer = myAgent.receive(mt2);
                    if (replyOffer != null) {
                        if (!oferty.containsKey(replyOffer.getSender() + replyOffer.getContent())) {
                            oferty.put(replyOffer.getSender() + replyOffer.getContent(), counter);
                            counter++;

                            double offerowanaCena = Double.parseDouble(replyOffer.getContent());
                            System.out.println("Nowa oferta : " + String.valueOf(offerowanaCena) + " numer: " + String.valueOf(counter));
                            if (counter > 7) {
                                System.out.println("Odrzucono kupno");
                                ACLMessage order2 = new ACLMessage(ACLMessage.CANCEL);
                                order2.addReceiver(bestSeller);
                                order2.setContent(targetBookTitle);
                                order2.setConversationId("book-trade");
                                order2.setReplyWith("order" + System.currentTimeMillis());
                                myAgent.send(order2);
                                myAgent.doDelete();
                                break;
                            }
                            if (offerowanaCena < (lastPrice - 3)) {
                                step = 2;
                                System.out.println("Akceptuje oferte");
                                bestPrice = (int) offerowanaCena;
                                break;
                            }
                            lastPrice = lastPrice + 6;
                            ACLMessage oferta = new ACLMessage(ACLMessage.REQUEST);
                            oferta.setContent(String.valueOf(lastPrice));
                            oferta.addReceiver(bestSeller);
                            oferta.setConversationId("Bidding" + String.valueOf(System.currentTimeMillis()));
                            myAgent.send(oferta);
                            System.out.println("Proponowana cena: " + lastPrice);
                        }
                    } else {
                        block();
                    }
                    break;
            }  // switch
        } // action

        public boolean done() {
            return ((step == 2 && bestSeller == null) || step == 4);
        }
    } // Koniec wewnętrznej klasy RequestPerformer
}
