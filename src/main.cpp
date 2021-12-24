
#include <windows.h>
#include <algorithm>
#include <memory>  // pentru allocator, __shared_ptr_access
#include <cstdlib>// pentru EXIT_SUCCESS
#include <string>  // pentru string, operator+ (supraincarcare), basic_string, to_string, char_traits
#include <vector>  // pentru vector, __alloc_traits<>::value_type
#include <stdio.h>

#include "ftxui/component/captured_mouse.hpp"    // pentru ftxui
#include "ftxui/component/component.hpp"         // pentru clasele Menu, Renderer, Horizontal, Vertical
#include "ftxui/component/component_base.hpp"    // pentru clasa ComponentBase
#include "ftxui/component/component_options.hpp" // pentru clasele InputOption, ButtonOption
#include "ftxui/component/screen_interactive.hpp"// pentru clasele Component, ScreenInteractive
#include "ftxui/dom/elements.hpp"                // pentru text, Element, operator| (supraincarcare), window, flex, vbox
#include "ftxui/util/ref.hpp"                    // pentru clasa Ref
#include <Python.h>
#include "pybind11/embed.h"

namespace py = pybind11;
using namespace py::literals;

#include "../lib/ArboreBinar.h"

using namespace ftxui;
using namespace std;


template<typename T>
vector<vector<T>> ImpartireVector(vector<T> el_parcurse, size_t nr_segmente)
{
    vector<vector<T>> vector_out;

    int lungime = el_parcurse.size() / nr_segmente;
    int rest = el_parcurse.size() % nr_segmente;

    int inceput = 0;
    int sfarsit = 0;

    for (int i = 0; i < min<int>(nr_segmente, el_parcurse.size()); i++)
    {
        sfarsit += lungime + (rest-- > 0);
        vector_out.push_back(vector<T>(el_parcurse.begin() + inceput, el_parcurse.begin() + sfarsit));
        inceput = sfarsit;
    }
    return vector_out;
}

Elements AfisareElementeParcurseImpartite(vector<vector<Element>> vector)
{
    Elements vector_final;
    vector_final.push_back(hbox(text(" ")));
    vector_final.push_back(hbox({text("Ordine noduri parcurse: ")}) | center);

    for (int i = 0; i < vector.size() - 1; i++)
    {
        vector_final.push_back(hbox(vector[i], text("→") | center) | center);
    }
    vector_final.push_back(hbox(vector.back()) | center);
    return vector_final;
}

// Struct folosit pentru comutarea dintre optiunile de Parcurgere Subarbore Nod Curent si Parcurgere Arbore(intreg)
struct CheckboxState
{
    bool marcat;
};

Elements ConversieStringMulti(string arbore, string delimiter)
{
    Elements stringuri;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = arbore.find(delimiter, prev)) != std::string::npos)
    {
        stringuri.push_back(text(arbore.substr(prev, pos - prev)));
        prev = pos + 1;
    }

    // To get the last substring (or only, if delimiter is not found)
    stringuri.push_back(text(arbore.substr(prev)));

    return stringuri;
}

int main(int argc, const char *argv[])
{
    SetConsoleOutputCP(65001);
    pybind11::scoped_interpreter guard{};

    auto sys = py::module::import("sys");
    auto modul_test = py::module::import("afisare_arbore");
    auto functie_convertita = modul_test.attr("main");
    auto preluat = functie_convertita();
    auto arbore_reprezentat_grafic = preluat.cast<string>();
    Elements vector_linii_arbore = ConversieStringMulti(arbore_reprezentat_grafic, "\n");

    ArboreBinar arbore_binar;

    auto screen = ScreenInteractive::Fullscreen();
    // Am declarat variabilele in care se vor stoca valorile aferente nodului curent
    string val_nod{}, val_copil_stang{}, val_copil_drept{};

    // Aici am creat componenta care stocheaza valoarea introdusa de la tastatura a nodului curent (in input)
    Component input_val_nod = Input(&val_nod, "editati valoarea nodului");

    //    Component input_copil_stang = Input(&val_copil_stang, "introduceti valoarea");
    //    Component input_copil_drept = Input(&val_copil_drept, "introduceti valoarea");

    // Sectiune in care se definesc proprietatile butonului de salvare din fereastra "Editare si Adaugare Nod Curent"
    auto optiune_buton_salv_nod = ButtonOption();
    optiune_buton_salv_nod.border = false;

    auto buton_salvare_nod = Container::Horizontal({
                                                           Button("[Salveaza si adauga nod]", [&]
                                                                  {
                                                                      arbore_binar.SalvareNod(val_nod, val_copil_stang, val_copil_drept);
                                                                      auto informatii_nod_crt = arbore_binar.GetInformatiiNodCurent();
                                                                      val_nod = informatii_nod_crt._nod_curent._informatie_nod;
                                                                      val_copil_stang = informatii_nod_crt._informatie_descendent_stang;
                                                                      val_copil_drept = informatii_nod_crt._informatie_descendent_drept;
                                                                  },
                                                                  &optiune_buton_salv_nod),
                                                   });

    // Sectiune in care se definesc meniurile de tip Pop-Up din sectiunea editare copii
    // Variabila "adancime" - se refera la nivelul la care operam.
    // 0 - reprezinta zona de dedesubt
    // 1 - reprezinta nivelul cu meniul "Adaugare copil stanga"
    // 2 - reprezinta nivelul cu meniul "Adaugare copil dreapta"

    // Am setat "depth-ul" sau adancimea default ca fiind 0.
    // Adica default-ul este afisarea ferestrelor principale.
    int adancime = 0;

    // Sectiune care se ocupa de butonul de salvare a copilului stang
    // Acest buton face posibila revenirea la adancimea 0 a programului
    auto optiune_buton_salv_copil_stang = ButtonOption();
    optiune_buton_salv_copil_stang.border = false;
    auto buton_salvare_copil_stang = Container::Horizontal({
                                                                   Button("[Salveaza copil stang]", [&]
                                                                          { adancime = 0; },
                                                                          &optiune_buton_salv_copil_stang),
                                                           });
    // sfarsit sectiune

    // Sectiune care se ocupa de butonul de salvare a copilului drept
    // Acest buton face posibila revenirea la adancimea 0 a programului
    auto optiune_buton_salv_copil_drept = ButtonOption();
    optiune_buton_salv_copil_drept.border = false;
    auto buton_salvare_copil_drept = Container::Horizontal({
                                                                   Button("[Salveaza copil drept]", [&]
                                                                          { adancime = 0; },
                                                                          &optiune_buton_salv_copil_drept),
                                                           });
    // sfarsit sectiune

    // Sectiune care se ocupa cu randarea meniului Pop-Up de pe nivelul 1
    auto container_adancime_1_st = Container::Vertical({
                                                               Input(&val_copil_stang, "copil stang"),
                                                               buton_salvare_copil_stang,
                                                       });

    auto randare_adancime_1_copil_st = Renderer(container_adancime_1_st, [&]
    {
        return vbox({hbox(text("Introduceti valoare: ")),
                     container_adancime_1_st->Render() | center}) | border;
    });
    // sfarsit sectiune

    // Sectiune care se ocupa cu randarea meniului Pop-Up de pe nivelul 2
    auto container_adancime_2_dr = Container::Vertical({
                                                               Input(&val_copil_drept, "copil drept"),
                                                               buton_salvare_copil_drept,
                                                       });

    auto randare_adancime_2_copil_dr = Renderer(container_adancime_2_dr, [&]
    {
        return vbox({hbox(text("Introduceti valoare: ")),
                     container_adancime_2_dr->Render() | center}) |
               border;
    });
    // sfarsit sectiune

    // Sectiune in care se definesc proprietatile butoanelor de salvare si adaugare ale copiilor nodului curent
    auto butoane_salvare_copii = Container::Horizontal({
                                                               Button(
                                                                       "[Copil stang]", [&]
                                                                       { adancime = 1; },
                                                                       &optiune_buton_salv_copil_stang),
                                                               Button(
                                                                       "[Copil drept]", [&]
                                                                       { adancime = 2; },
                                                                       &optiune_buton_salv_copil_drept),
                                                       });
    // sfarsit sectiune

    // Aici se definesc proprietatile care vor fi "pasate" in componenta de editare a nodului


    // Sectiune care se ocupa cu randarea ferestrei Detalii Arbore

    auto tab_detalii_arbore = Renderer([&]
                                       {
                                           auto statistici = arbore_binar.GetStatisticiArbore();
                                           return window(text("Detalii Arbore"),//
                                                         vbox({hbox(text(" ")),
                                                               hbox(text("Nr. niveluri: " + statistici._numar_niveluri)),
                                                               hbox(text("Nr. noduri: " + statistici._numar_noduri)),
                                                               hbox(text("Nr. frunze: " + statistici._numar_frunze)),
                                                               hbox(text(" "))
                                                              })
                                           ) | size(ftxui::WIDTH, ftxui::EQUAL, 40);
                                       });
    // Sectiune care se ocupa cu randarea ferestrei Detalii Nod Curent
    auto tab_detalii_nod = Renderer([&]
                                    {
                                        return window(text("Detalii Nod Curent"),//
                                                      vbox({
                                                                   hbox(text(" ")),
                                                                   hbox(text(val_nod.empty() ? "Nod curent: - " :
                                                                             "Nod curent: "), text(val_nod) | color(Color::GreenLight)),
                                                                   hbox(text(
                                                                           val_copil_stang.empty() ? "Copil stang: - " :
                                                                           "Copil stang: " + val_copil_stang)),
                                                                   hbox(text(
                                                                           val_copil_drept.empty() ? "Copil drept: - " :
                                                                           "Copil drept: " + val_copil_drept)),
                                                                   hbox(text(" ")),
                                                           })
                                        ) | size(ftxui::WIDTH, ftxui::EQUAL, 40);
                                    });
    // Sectiune care se ocupa cu randarea ferestrei de Editare si Adaugare Nod Curent
    // Aici se definesc proprietatile care vor fi "pasate" in componenta de editare a copiilor nodului curent
    auto proprietati_nod = Container::Vertical({
                                                       input_val_nod,
                                                       buton_salvare_nod,
                                                       butoane_salvare_copii
                                               });
    // Sectiune care se ocupa cu randarea ferestrei de Editare si Adaugare Copii Nod Curent
    auto tab_editare_copii = Renderer(proprietati_nod, [&]
    {

        return window(text("Editare Nod Curent"),//
                      vbox({
                                   hbox(text(" ")),
                                   hbox(text("Nod Crt.: "), input_val_nod->Render()),
                                   hbox(text(" ")),
                                   separator(),
                                   hbox({val_nod.empty() ? text("* Completati campul \"Nod Crt.\"") | color(Color::RedLight)
                                                         : buton_salvare_nod->Render()}) | center,

                                   separator(),
                                   hbox(text(val_copil_stang.empty() && val_copil_drept.empty()
                                             ? "* Nodul curent nu are copii (frunza)"
                                             : "")) | color(Color::GreenLight),
                                   hbox(text(val_copil_stang.empty() ? "* Nu exista copil stang " : "Copil stang: " +
                                                                                                    val_copil_stang)),

                                   hbox(text(val_copil_drept.empty() ? "* Nu exista copil drept " : "Copil drept: " +
                                                                                                    val_copil_drept)),
                                   hbox(text(" ")),
                                   separator(),
                                   hbox({val_nod.empty() ? text("* Completati campul \"Nod Crt.\"") | color(Color::RedLight)
                                                         : butoane_salvare_copii->Render()}) | center
                           })
        ) | size(ftxui::WIDTH, ftxui::EQUAL, 40);
    });

    // Sectiune care se ocupa cu randarea meniului cu traversari
    int parcurgere_selectata = 0;
    MenuOption optiuni_meniu;
    optiuni_meniu.style_selected = color(Color::CadetBlue);
    optiuni_meniu.style_focused = bgcolor(Color::CadetBlue);
    optiuni_meniu.style_selected_focused = bgcolor(Color::CadetBlue);

    CheckboxState stare_1{};
    CheckboxState stare_2{};
    bool schimbat = false;
    vector<string> elemente_meniu_parcurgeri = {"Preordine", "Inordine", "Postordine", "In Latime"};


    auto container_meniu_parcurgeri = Container::Vertical({});
    container_meniu_parcurgeri->Add(Menu(&elemente_meniu_parcurgeri, &parcurgere_selectata, &optiuni_meniu));

    auto optiuni_parcurgeri = Renderer(container_meniu_parcurgeri, [&]
    {
        return vbox({hbox(text(" ")),
                     hbox(vbox(text("Selectati parcurgerea: ")),
                          vbox({container_meniu_parcurgeri->Render()})),
                     hbox(text(" ")),
                     separator()
                    });
    });


    auto container_checkboxuri = Container::Vertical({});
    container_checkboxuri->Add(Checkbox("parcurgeti intreg arborele", &stare_1.marcat));
    container_checkboxuri->Add(Checkbox("parcurgeti subarborele nodului curent", &stare_2.marcat));

    auto meniu_checkboxuri = Renderer(container_checkboxuri, [&]
    {
        return vbox(container_checkboxuri->Render() | frame |
                    size(HEIGHT, LESS_THAN, 10));
    });

    auto meniu_final_parcurgeri = Container::Vertical({
                                                              optiuni_parcurgeri,
                                                              meniu_checkboxuri
                                                      });


// Sectiune care se ocupa cu randarea tab-ului de reprezentare grafica a arborelui binar
    auto reprezentare_grafica = Renderer(meniu_final_parcurgeri, [&]
    {
        if (stare_1.marcat && !schimbat)
        {
            schimbat = true;
            if (schimbat)
            {
                stare_2.marcat = false;
            }
        } else if (stare_2.marcat && schimbat)
        {
            schimbat = false;
            if (!schimbat)
            {
                stare_1.marcat = false;
            }
        } else if (!stare_1.marcat && !stare_2.marcat)
        {
            stare_1.marcat = true;
        }

        auto informatii_nod = arbore_binar.GetInformatiiNodCurent();

        auto vector_noduri_parcurse = arbore_binar.GetNodDtoInitVect();

        if (!informatii_nod._nod_curent._informatie_nod.empty())
            switch (parcurgere_selectata)
            {
                case 0:
                {
                    if (stare_1.marcat)
                        vector_noduri_parcurse = arbore_binar.ParcurgerePreordineDeLaRadacina();
                    else if (stare_2.marcat)
                        vector_noduri_parcurse = arbore_binar.ParcurgerePreordineDeLaNodulCurent();
                    break;
                }
                case 1:
                {
                    if (stare_1.marcat)
                        vector_noduri_parcurse = arbore_binar.ParcurgereInordineDeLaRadacina();
                    else if (stare_2.marcat)
                        vector_noduri_parcurse = arbore_binar.ParcurgereInordineDeLaNodulCurent();
                    break;
                }
                case 2:
                {
                    if (stare_1.marcat)
                        vector_noduri_parcurse = arbore_binar.ParcurgerePostordineDeLaRadacina();
                    else if (stare_2.marcat)
                        vector_noduri_parcurse = arbore_binar.ParcurgerePostordineDeLaNodulCurent();
                    break;
                }
                case 3:
                {
                    if (stare_1.marcat)
                        vector_noduri_parcurse = arbore_binar.ParcurgereInLatimeDeLaRadacina();
                    else if (stare_2.marcat)
                        vector_noduri_parcurse = arbore_binar.ParcurgereInLatimeDeLaNodulCurent();
                    break;
                }
                default:
                {
                    throw std::logic_error("Eroare fatala!");
                }
            }

//        if (parcurgere_selectata == 0 && !informatii_nod._nod_curent._informatie_nod.empty() && stari[0].marcat)
//        {
//            vector_noduri_parcurse = arbore_binar.ParcurgerePreordineDeLaRadacina();
//        } else if (parcurgere_selectata == 1 && !informatii_nod._nod_curent._informatie_nod.empty() && stari[0].marcat)
//        {
//            vector_noduri_parcurse = arbore_binar.ParcurgereInordineDeLaRadacina();
//        } else if (parcurgere_selectata == 2 && !informatii_nod._nod_curent._informatie_nod.empty() && stari[0].marcat)
//        {
//            vector_noduri_parcurse = arbore_binar.ParcurgerePostordineDeLaRadacina();
//        } else if (parcurgere_selectata == 3 && !informatii_nod._nod_curent._informatie_nod.empty() && stari[0].marcat)
//        {
//            vector_noduri_parcurse = arbore_binar.ParcurgereInLatimeDeLaRadacina();
//        } else if (parcurgere_selectata == 0 && !informatii_nod._nod_curent._informatie_nod.empty() && stari[1].marcat)
//        {
//            vector_noduri_parcurse = arbore_binar.ParcurgerePreordineDeLaNodulCurent();
//
//        } else if (parcurgere_selectata == 1 && !informatii_nod._nod_curent._informatie_nod.empty() && stari[1].marcat)
//        {
//            vector_noduri_parcurse = arbore_binar.ParcurgereInordineDeLaNodulCurent();
//        } else if (parcurgere_selectata == 2 && !informatii_nod._nod_curent._informatie_nod.empty() && stari[1].marcat)
//        {
//            vector_noduri_parcurse = arbore_binar.ParcurgerePostordineDeLaNodulCurent();
//        } else if (parcurgere_selectata == 3 && !informatii_nod._nod_curent._informatie_nod.empty() && stari[1].marcat)
//        {
//            vector_noduri_parcurse = arbore_binar.ParcurgereInLatimeDeLaNodulCurent();
//        }


        Elements elemente_parcurse, vector_final;
        size_t parte_intreaga{};
        if (!informatii_nod._nod_curent._informatie_nod.empty() && !vector_noduri_parcurse.empty())
        {
            if (vector_noduri_parcurse.front()._id_nod == informatii_nod._nod_curent._id_nod)
                elemente_parcurse.push_back(
                        hbox(hbox(text(vector_noduri_parcurse.front()._informatie_nod))
                             | border
                             | color(Color::GreenLight)
                             | center)
                );
            else
            {
                elemente_parcurse.push_back(hbox((text(vector_noduri_parcurse.front()._informatie_nod)) | border
                                                 | center)
                );
            }

            for (int i = 1; i < vector_noduri_parcurse.size(); i++)
            {
                if (vector_noduri_parcurse[i]._id_nod == informatii_nod._nod_curent._id_nod)
                    elemente_parcurse.push_back(hbox({text("→") | center,
                                                      hbox(text(vector_noduri_parcurse[i]._informatie_nod)) | border |
                                                      color(Color::GreenLight) | center}));
                else
                {
                    elemente_parcurse.push_back(
                            hbox({text("→") | center, hbox(text(vector_noduri_parcurse[i]._informatie_nod)) | border | center}));
                }
            }

            parte_intreaga = elemente_parcurse.size() / 10;

            if (parte_intreaga > 1)
            {
                vector<vector<Element>> vector_out = ImpartireVector(elemente_parcurse, parte_intreaga);
                vector_final = AfisareElementeParcurseImpartite(vector_out);
            } else
            {
                vector_final = elemente_parcurse;
            }
        }

        return window(text("Arbore Binar - Reprezentare Grafica"),
                      vbox({

                                   filler(),
                                   vbox(vector_linii_arbore) | center,
                                   filler(),
                                   !informatii_nod._nod_curent._informatie_nod.empty() ?
                                   hbox({
                                                window(text("Meniu Parcurgere"),
                                                       hbox({meniu_final_parcurgeri->Render()
                                                             | size(ftxui::WIDTH, ftxui::GREATER_THAN, 70),
                                                             separator(),
                                                             hbox({
                                                                          hbox({hbox(parte_intreaga > 1 ?
                                                                                     vbox(vector_final)
                                                                                     | size(ftxui::WIDTH, ftxui::GREATER_THAN, 115)
                                                                                                        :
                                                                                     vbox({hbox(text(" ")),
                                                                                           hbox({text(
                                                                                                   "Ordine noduri parcurse: ")}) |
                                                                                           center,
                                                                                           hbox({vector_final}) |
                                                                                           center

                                                                                          })
                                                                                     | size(ftxui::WIDTH, ftxui::GREATER_THAN, 135))
                                                                               })
                                                                  })
                                                            })
                                                )}
                                   )
                                                                                       : hbox({})}
                      ))
               | size(ftxui::WIDTH,
                      ftxui::GREATER_THAN,
                      185);
    });

// Sectiune care se ocupa cu randarea intregului nivel de adancime 0 (Principal)
    auto container_adancime_0 = Container::Vertical(
            {tab_detalii_arbore, tab_detalii_nod, tab_editare_copii,
             reprezentare_grafica});
    auto randare_adancime_0 = Renderer(container_adancime_0, [&]
    {
        return hbox({hbox({vbox({tab_detalii_arbore->Render(),
                                 tab_detalii_nod->Render(),
                                 tab_editare_copii->Render(),
                                })
                          }),
                     hbox({reprezentare_grafica->Render()}) | flex
                    });
    });

// Containerul principal tine toate elementele de pe toate nivelurile
    auto container_principal = Container::Tab({randare_adancime_0,
                                               randare_adancime_1_copil_st,
                                               randare_adancime_2_copil_dr
                                              }, &adancime);
// Sectiune in care se randeaza containerul principal
    auto randare_principala = Renderer(container_principal, [&]
    {
        Element document = randare_adancime_0->Render();

        if (adancime == 1 && !val_nod.empty())
        {
            document = dbox({document,
                             randare_adancime_1_copil_st->Render() | clear_under | center,
                            });
        } else if (adancime == 2 && !val_nod.empty())
        {
            document = dbox({document,
                             randare_adancime_2_copil_dr->Render() | clear_under | center,
                            });
        }
        return document;
    });

// Aici se face loop-ul bufferului de afisare pentru actualizarea in timp real

    screen.Loop(randare_principala);
    return EXIT_SUCCESS;
}
